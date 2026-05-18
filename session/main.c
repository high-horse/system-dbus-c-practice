#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>
#include <dbus/dbus.h>


#define PORTAL_BUS "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_IFACE "org.freedesktop.portal.Screenshot"
#define TIMEOUT 1000

void error_check(DBusError *error, const char *msg);
char *get_parent_window(void);
char *get_active_x11_window(void);


int main() {
    DBusConnection *connection;
    DBusError error;
    DBusMessage *request, *reply;
    DBusMessageIter iter, sub;

    const char *parent_window = get_parent_window();
    // const char *parent_window = "x11:0x2c0003d";  // from xprop _NET_ACTIVE_WINDOW
    char *request_handle = NULL;

    dbus_error_init(&error);

    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    error_check(&error, "dbus connection error");

    request = dbus_message_new_method_call(PORTAL_BUS, PORTAL_PATH, PORTAL_IFACE, "Screenshot");
    if(!request) {
        fprintf(stderr, "Failed to create reqest message\n");
        goto err_cleanup;
    }

    dbus_message_iter_init_append(request, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &parent_window);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &sub);
    dbus_message_iter_close_container(&iter, &sub);


    reply = dbus_connection_send_with_reply_and_block(connection, request, TIMEOUT, &error);
    error_check(&error, "FAILED TO GET REPLY");
    dbus_message_unref(request);


    if(!dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &request_handle, DBUS_TYPE_INVALID)) {
        dbus_message_unref(reply);
        error_check(&error, "FAILED TO PARSE REPLY ARGS");
    }


    printf("Portal spawned Request Handle: %s\n", request_handle);
    dbus_message_unref(reply);


    char match_rule[512]; // Increased size safely to fit long object paths
    // snprintf(
    //     match_rule, 
    //     sizeof(match_rule), 
    //     "type='signal',sender='org.freedesktop.portal.Desktop',interface='org.freedesktop.portal.Request',path='%s',member='Response'", 
    //     request_handle
    // );

    snprintf(
        match_rule, 
        sizeof(match_rule), 
        "type='signal',interface='org.freedesktop.portal.Request',path='%s',member='Response'", 
        request_handle
    );

    dbus_bus_add_match(connection, match_rule, &error);
    dbus_connection_flush(connection);
    error_check(&error, "FAILED TO AdD mATCH OR FLUSH CONNECTION");

    printf("waiting for user interaction via desktop portal...\n");

    
    while(dbus_connection_read_write_dispatch(connection, TIMEOUT)) {
            printf("inside main while\n");

        DBusMessage *sig_msg = dbus_connection_pop_message(connection);
        if(!sig_msg) continue;

        printf("Got message: interface=%s, member=%s, path=%s\n",
           dbus_message_get_interface(sig_msg) ?: "(none)",
           dbus_message_get_member(sig_msg) ?: "(none)",
           dbus_message_get_path(sig_msg) ?: "(none)");

        if(dbus_message_is_signal(sig_msg, "org.freedesktop.portal.Request", "Response")) {  

            printf("inside level 2 while\n");

            DBusMessageIter sig_iter, res_sig_iter;
            dbus_uint32_t resp_code;
            
            dbus_message_iter_init(sig_msg, &sig_iter);
            dbus_message_iter_get_basic(&sig_iter, &resp_code); // 0 success, 1 failure

            printf("got response code %d\n", resp_code);

            if(resp_code == 0 && dbus_message_iter_next(&sig_iter)) {
            printf("inside level 3 while\n");

                dbus_message_iter_recurse(&sig_iter, &res_sig_iter);

                while (dbus_message_iter_get_arg_type(&res_sig_iter) == DBUS_TYPE_DICT_ENTRY)
                {
                    DBusMessageIter entry_iter, var_iter;
                    const char *key;
                    dbus_message_iter_recurse(&res_sig_iter, &entry_iter);
                    dbus_message_iter_get_basic(&entry_iter, &key);

                    if(strcmp(key, "uri") == 0) {
                        const char *uri;
                        dbus_message_iter_next(&entry_iter);
                        dbus_message_iter_recurse(&entry_iter, &var_iter);
                        dbus_message_iter_get_basic(&var_iter, &uri);

                        printf("\nSuccess! Screenshot saved at: %s\n", uri);
                        break;
                    }
                    dbus_message_iter_next(&res_sig_iter);
                }
                

                // dbus_message_iter_open_container(&sig_iter, DBUS_TYPE_ARRAY, "{sv}", &res_sig_iter);

                // while (dbus_message_iter_get_arg_type(&res_sig_iter) == DBUS_TYPE_DICT_ENTRY)
                // {
                //     DBusMessageIter entry_iter, var_iter;
                //     const char *key;
                //     dbus_message_iter_recurse(&res_sig_iter, &entry_iter);
                //     dbus_message_iter_get_basic(&entry_iter, &key);

                //     if(strcmp(key, "uri") == 0) {
                //         const char *uri;
                //         dbus_message_iter_next(&entry_iter);
                //         dbus_message_iter_recurse(&entry_iter, &var_iter);
                //         dbus_message_iter_get_basic(&var_iter, &uri);

                //         printf("\nSuccess! Screenshot saved at: %s\n", uri);
                //         break;
                //     }

                //     dbus_message_iter_next(&res_sig_iter);
                // }
                
            } else {
                printf("\nScreenshot requested was canceled or failed.\n");
            }

            dbus_message_unref(sig_msg);
            break;

        }
        dbus_message_unref(sig_msg);
    }

    goto ok_cleanup;

    ok_cleanup:
        dbus_connection_unref(connection);
        return EXIT_SUCCESS;

    err_cleanup:
        dbus_connection_unref(connection);
        return EXIT_FAILURE;
}

void error_check(DBusError *error, const char *msg) {
    if(dbus_error_is_set(error)) {
        fprintf(stderr, "%s: %s", msg, error->message);
        dbus_error_free(error);
        exit(EXIT_FAILURE);
    }
}


char *get_parent_window(void) {
    const char *wayland_display = getenv("WAYLAND_DISPLAY");
    const char *display = getenv("DISPLAY");
    
    if (wayland_display && *wayland_display) {
        // Wayland: no easy way to get handle from plain C
        // Options:
        // 1. Return "" (may fail with AccessDenied)
        // 2. Use GTK/Qt which handles this internally
        // 3. Parse WAYLAND_DISPLAY (not a window handle)
        return "";
    }
    
    if (display && *display) {
        return get_active_x11_window();
        // X11: query _NET_ACTIVE_WINDOW via Xlib or xprop
        // For now, hardcode or use Xlib to query dynamically
        return "x11:0x2c0003d";  // Or query dynamically
    }

    return "";

}

char *get_active_x11_window(void) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return NULL;
    
    Window root = DefaultRootWindow(dpy);
    Atom prop = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;
    
    if (XGetWindowProperty(dpy, root, prop, 0, 1, False, AnyPropertyType,
                           &type, &format, &nitems, &bytes_after, &data) == Success 
        && nitems > 0) {
        Window *win = (Window *)data;
        char *result = malloc(64);
        snprintf(result, 64, "x11:0x%lx", *win);
        XFree(data);
        XCloseDisplay(dpy);
        return result;
    }
    
    XCloseDisplay(dpy);
    return NULL;
}