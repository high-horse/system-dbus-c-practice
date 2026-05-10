#include<stdio.h>
#include<stdlib.h>
#include <dbus/dbus.h>

static void check_and_abort(DBusError *error) ;
/*
a DBUS server application has a well-known name (ex. org.freedesktop.UPower);
a DBUS server application exposes one on more objects (ex. /org/freedesktop/UPower, /org/freedesktop/UPower/devices/BAT_1);
any object implements one or more interfaces (ex. org.freedesktop.DBus.Properties).
*/
int main() {
    DBusConnection *connection = NULL;
    DBusError error;
    DBusMessage *msgQuery = NULL;
    DBusMessage *msgReply = NULL;
    const char *interfaceName = NULL;
    const char *versionValue = NULL;


    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    check_and_abort(&error);

    interfaceName = "org.freedesktop.UPower";
    // The dbus_message_new_method_call is used to create a new message 
    // that must be sent to the bus. 
    // The arguments are the coordinates of the process,
    //  the object, the interface and the method to call:
    msgQuery = dbus_message_new_method_call(
        interfaceName,
        "/org/freedesktop/UPower",
        "org.freedesktop.UPower",
        "GetCriticalAction"
    );


    // When the message is created the function 
    // dbus_connection_send_with_reply_and_block will send the message on the bus
    // and wait for the response of the other application for one second
    // (the 1000 in argument list). The function returns the reply:
    msgReply = dbus_connection_send_with_reply_and_block(connection, msgQuery, 1000, &error);
    check_and_abort(&error);

    // Every DBus message created or returned must be deallocated with the dbus_message_unref call:
    dbus_message_unref(msgQuery);

    // Now we must extract the result string from the reply message 
    // and we do it with dbus_message_get_args, which is ok for basic types:
    dbus_message_get_args(msgReply, &error, DBUS_TYPE_STRING, &versionValue, DBUS_TYPE_INVALID);
    printf("the critical action is: %s \n", versionValue);
    dbus_message_unref(msgReply);
    return 0;
}

static void check_and_abort(DBusError *error) {
    if(!dbus_error_is_set(error)) return;

    puts(error->message);
    abort();
}