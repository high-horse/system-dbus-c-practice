#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define BUS_NAME "org.freedesktop.UPower"
#define BUS_PATH "/org/freedesktop/UPower/devices/battery_BAT1"
#define BUS_IFACE "org.freedesktop.UPower.Device"
#define TIMEOUT 1000

static void abort_on_error(DBusError *error) ;
static DBusMessage *create_property_get_message(const char *bus_name, const char *path, const char *iface, const char *propname);
static double extract_double_from_variant(DBusMessage *reply, DBusError *error);
static double get_double_property(DBusConnection *connection, const char *bus_name, const char *path, const char *iface, const char *propname, DBusError *error);

int main() {
    DBusConnection *connection = NULL;
    DBusError error;
    double energy = 0;
    double fullEnergy = 0;

    dbus_error_init(&error);

    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    abort_on_error(&error);

    energy = get_double_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "Energy", &error);
    abort_on_error(&error);

    fullEnergy = get_double_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "EnergyFull", &error);
    abort_on_error(&error);

    printf("%lf \n", (energy * 100) / fullEnergy);
    return EXIT_SUCCESS;
}

static void abort_on_error(DBusError *error) {
    if(dbus_error_is_set(error)) {   
        fprintf(stderr, "%s", error->message);
        abort();
    }
}

static double get_double_property(
    DBusConnection *connection,
    const char *bus_name,
    const char *path,
    const char *iface, 
    const char *propname,
    DBusError *error
) {
    DBusError myerror;
    double result = 0;
    DBusMessage *queryMessage = NULL;
    DBusMessage *replyMessage = NULL;

    dbus_error_init(&myerror);

    queryMessage = create_property_get_message(bus_name, path, iface, propname);
    replyMessage = dbus_connection_send_with_reply_and_block(connection, queryMessage, TIMEOUT, &myerror);
    dbus_message_unref(queryMessage);

    if(dbus_error_is_set(&myerror)) {
        dbus_move_error(&myerror, error);
        return EXIT_FAILURE;
    }

    result = extract_double_from_variant(replyMessage, &myerror);
    if(dbus_error_is_set(&myerror)){
        dbus_move_error(&myerror, error);
        return EXIT_FAILURE;
    }

    dbus_message_unref(replyMessage);
    return result;
}


static DBusMessage *create_property_get_message(
    const char *bus_name, 
    const char *path, 
    const char *iface, 
    const char *propname
){
    DBusMessage *queryMessage = dbus_message_new_method_call(
        bus_name, path, "org.freedesktop.DBus.Properties", "Get"
    );

    dbus_message_append_args(queryMessage, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &propname, DBUS_TYPE_INVALID);

    return queryMessage;
}

static double extract_double_from_variant(DBusMessage *reply, DBusError *error){
    DBusMessageIter iter;
    DBusMessageIter sub;
    double result;

    dbus_message_iter_init(reply, &iter);

    if(DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter)){
        dbus_set_error_const(error, "reply_should_be_variant", "this message hasn't a variant message type");
        return EXIT_SUCCESS;
    }

    dbus_message_iter_recurse(&iter, &sub) ;
    if(DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&sub)){
        dbus_set_error(error, "variant_should be double", "the reply message must be double content");
        return EXIT_SUCCESS;
    }

    dbus_message_iter_get_basic(&sub, &result);
    return result;
}