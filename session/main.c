#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>


#define PORTAL_BUS "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_IFACE "org.freedesktop.portal.Screenshot"

void error_check(DBusError *error, const char *msg);

int main() {
    DBusConnection *connection;
    DBusError error;
    DBusMessage *request, *reply;
    DBusMessageIter iter, sub;

    const char *parent_window = "";
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


    reply = dbus_connection_send_with_reply_and_block(connection, request, -1, &error);
    error_check(&error, "FAILED TO GET REPLY");
    dbus_message_unref(request);

    if(!dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &request_handle, DBUS_TYPE_INVALID)) {
        dbus_message_unref(reply);
        error_check(&error, "FAILED TO PARSE REPLY ARGS");
    }

    printf("Portal spawned Request Handle: %s\n", request_handle);
    dbus_message_unref(reply);

    char match_rule[512]; // Increased size safely to fit long object paths
    snprintf(
        match_rule, 
        sizeof(match_rule), 
        "type='signal',sender='org.freedesktop.portal.Desktop',interface='org.freedesktop.portal.Request',path='%s',member='Response'", 
        request_handle
    );

    // snprintf(
    //     match_rule, 
    //     sizeof(match_rule), 
    //     "type='signal',interface='org.freedesktop.portal.Request',path='%s',member='Response'", 
    //     request_handle
    // );

    dbus_bus_add_match(connection, match_rule, &error);
    dbus_connection_flush(connection);
    error_check(&error, "FAILED TO AdD mATCH OR FLUSH CONNECTION");

    printf("waiting for user interaction via desktop portal...\n");

    while(dbus_connection_read_write_dispatch(connection, -1)) {
        DBusMessage *sig_msg = dbus_connection_pop_message(connection);
        if(!sig_msg) continue;

        if(dbus_message_is_signal(sig_msg, "org.freedesktop.portal.Request", "Response")) {            
            DBusMessageIter sig_iter, res_sig_iter;
            dbus_uint32_t resp_code;
            
            dbus_message_iter_init(sig_msg, &sig_iter);
            dbus_message_iter_get_basic(&sig_iter, &resp_code); // 0 success, 1 failure

            if(resp_code == 0 && dbus_message_iter_next(&sig_iter)) {
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
