#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dbus/dbus.h>

#define BUS_NAME "org.freedesktop.portal.Desktop"
#define BUS_PATH "/org/freedesktop/portal/desktop"
#define BUS_IFACE "org.freedesktop.DBus.Properties"
#define TIMEOUT 1000

void check_and_abort(DBusError *error, const char *message) {
    if (dbus_error_is_set(error))
    {
        fprintf(stderr, "%s : %s\n", message, error->message);
        abort();
    }
}

int main() {
    DBusConnection *connection = NULL;
    DBusError error;

    dbus_error_init(&error);
    check_and_abort(&error, "failed to init error");

    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    check_and_abort(&error, "failed to create connection");

    DBusMessage *request = dbus_message_new_method_call(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "CreateSession"
    );

    DBusMessageIter iter, sub;
    dbus_message_iter_init(request, &iter);

    dbus_message_iter_open_container(
        &iter, 
        DBUS_TYPE_ARRAY,
        "{sv}",
        &sub
    );
    dbus_message_iter_close_container(&iter, &sub);

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        connection, request, TIMEOUT, &error
    );
    check_and_abort(&error, " failed to create reply");

    DBusMessageIter rep_iter;
    dbus_message_iter_init(reply, &rep_iter);
    char *req_path = NULL;
    dbus_message_iter_get_basic(&iter, req_path);

    dbus_bus_add_match(
        connection, 
        "type='signal',"
        "interface='org.freedesktop.portal.Request',"
        "member='Response'",
        &error
    );
    check_and_abort(&error, "failed to add match");


    bool run = true;
    while (run)
    {
        dbus_connection_read_write(connection, TIMEOUT);

        DBusMessage *signal = dbus_connection_pop_message(connection);
        if(!signal) continue;
 
        if(dbus_message_is_signal(signal, "org.freedesktop.portal.Request", "Response")) {
            break;
        }
        dbus_message_unref(signal);
    }
    

}
