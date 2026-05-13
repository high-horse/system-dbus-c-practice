#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_and_abort(DBusError *error);
static DBusHandlerResult tutorial_message(DBusConnection *connection, DBusMessage *message, void *user_data);
static void respond_to_introspect(DBusConnection *connction, DBusMessage *request);
static void respond_to_sum(DBusConnection *connection, DBusMessage *request);

int main(void) {
    DBusConnection *connection;
    DBusError error;
    DBusObjectPathVTable vtable;


    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    check_and_abort(&error);

    dbus_bus_request_name(connection, "it.interface.camel.DBusTutorial", 0, &error);
    check_and_abort(&error);

    vtable.message_function = tutorial_message;
    vtable.unregister_function = NULL;

    dbus_connection_try_register_object_path(
        connection, 
        "it/interface/camel/DBusTutorial",
        &vtable,
        NULL, &error
    );
    check_and_abort(&error);

    while(TRUE) {
        dbus_connection_read_write_dispatch(
            connection, 1000
        );
    }
    return EXIT_SUCCESS;
} 

static void check_and_abort(DBusError *error) {
    if(dbus_error_is_set(error)) {
        puts(error->message);
        abort();
    }
}



/*
This is the heart of our server. This function reads the message using
dbus_message_get_interface and
dbus_message_get_member and then routes the message to the
appropriate function. If this function returns
“DBUS_HANDLER_RESULT_HANDLED” then the DBus library doesn’t consider
other handlers, instead if the functions returns
“DBUS_HANDLER_RESULT_NOT_YET_HANDLED” then the library will process
other handlers.
*/
static DBusHandlerResult tutorial_message(DBusConnection *connection, DBusMessage *message, void *user_data){
    const char *interface_name = dbus_message_get_interface(message);
    const char *member_name = dbus_message_get_number(message);

    if(
        0 == strcmp("org.freedesktop.DBus.Introspectable", interface_name) &&
        0 == strcmp("Introspect", member_name)
    ){
        respond_to_introspect(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if(
        0 == strcmp("it.interface.camel.DBusTutorial", interface_name) &&
        0 == strcmp("Sum", member_name)
    ) {
        respond_to_sum(connection, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
}


static void respond_to_introspect(DBusConnection *connction, DBusMessage *request) {
    DBusMessage *reply;

    const char *intospection_data = 
		" <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
		"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
		" <!-- dbus-sharp 0.8.1 -->"
        "   <interface name=\"org.freedesktop.DBus.Introspectable\">"
		"     <method name=\"Introspect\">"
		"       <arg name=\"data\" direction=\"out\" type=\"s\" />"
		"     </method>"
		"   </interface>"
		"   <interface name=\"it.interface.camel.DBusTutorial\">"
		"     <method name=\"Sum\">"
		"       <arg name=\"a\" direction=\"in\" type=\"i\" />"
		"       <arg name=\"b\" direction=\"in\" type=\"i\" />"
		"       <arg name=\"ret\" direction=\"out\" type=\"i\" />"
		"     </method>"
		"   </interface>"
		" </node>";

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(
        reply,
        DBUS_TYPE_STRING, &intospection_data,
        DBUS_TYPE_INVALID
    );
    dbus_connection_send(connction, reply, NULL);
    dbus_message_inref(reply);
}

static void respond_to_sum(DBusConnection *connection, DBusMessage *request) {
    DBusMessage *reply;
    DBusError error;
    int a = 0, b = 0, ret = 0;

    dbus_error_init(&error);

    dbus_message_get_args(
        reply, 
        &error, 
        DBUS_TYPE_INT32, &a,
        DBUS_TYPE_INT32, &b,
        DBUS_TYPE_INVALID
    );

    if(dbus_error_is_set(&error)) {
        reply = dbus_message_new_error(request, "wrong_arguments", "illegal argument to sum");
        dbus_connection_send(connection, reply, NULL);
        dbus_connection_unref(reply);
    }

    ret = a + b;

    reply = dbus_message_new_method_return(request);
    dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
}

