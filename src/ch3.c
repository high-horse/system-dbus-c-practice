#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#define BUS_NAME "org.freedesktop.UPower"
#define BUS_PATH "/org/freedesktop/UPower/devices/battery_BAT1"
#define BUS_IFACE "org.freedesktop.UPower.Device"
#define TIMEOUT 1000

typedef enum RETURN_TYPE {
    RETURN_UINT32,
    RETURN_INT32,
    RETURN_DOUBLE,
} RETURN_TYPE;

static void abort_on_error(DBusError *error) ;
static DBusMessage *create_property_get_message(const char *bus_name, const char *path, const char *iface, const char *propname);
static double extract_double_from_variant(DBusMessage *reply, DBusError *error);
static double get_double_property(DBusConnection *connection, const char *bus_name, const char *path, const char *iface, const char *propname, DBusError *error);

static int get_i32_property(DBusConnection *connection, const char *bus_name, const char *path, const char *iface, const char *propname, DBusError *error);
static int extract_i32_from_variant(DBusMessage *reply, DBusError *error);

static unsigned int get_ui32_property(DBusConnection *connection, const char *bus_name, const char *path, const char *iface, const char *propname, DBusError *error);
static unsigned int extract_ui32_from_variant(DBusMessage *reply, DBusError *error);


int main() {
    DBusConnection *connection = NULL;
    DBusError error;
    double energy = 0;
    double fullEnergy = 0;
    int chargeCycle = 0;

    dbus_error_init(&error);

    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    abort_on_error(&error);

    energy = get_double_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "Energy", &error);
    abort_on_error(&error);

    fullEnergy = get_double_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "EnergyFull", &error);
    abort_on_error(&error);

    chargeCycle = get_i32_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "ChargeCycles", &error);
    abort_on_error(&error);

    double health = get_double_property(connection, BUS_NAME, BUS_PATH, BUS_IFACE, "Capacity", &error);
    abort_on_error(&error); 


    printf("charge cycle %d\tbattery health %.2f\n", chargeCycle, health);
    printf("energy  %lf \n", (energy * 100) / fullEnergy);
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
        return -1;
    }

    result = extract_double_from_variant(replyMessage, &myerror);
    if(dbus_error_is_set(&myerror)){
        dbus_move_error(&myerror, error);
        return -1;
    }

    dbus_message_unref(replyMessage);
    return result;
}

/*
    NOTE: the property are to be accessed with generic `Properties` interface like `UPower.Properties` get method
    even though the property lies in other interface like `UPower.Device`
    pass the property to args with append_args 
*/

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
        return -1;
    }

    dbus_message_iter_recurse(&iter, &sub) ;
    if(DBUS_TYPE_DOUBLE != dbus_message_iter_get_arg_type(&sub)){
        dbus_set_error(error, "variant_should be double", "the reply message must be double content");
        return -1;
    }

    dbus_message_iter_get_basic(&sub, &result);
    return result;
}


static int get_i32_property(
    DBusConnection *connection, 
    const char *bus_name, 
    const char *path, 
    const char *iface, 
    const char *propname, 
    DBusError *error
) {
    DBusError myError;
    int result = 0;
    DBusMessage *queryMessage = NULL;
    DBusMessage *replyMessage = NULL;

    dbus_error_init(&myError);
    queryMessage = create_property_get_message(bus_name, path, iface, propname);
    replyMessage  = dbus_connection_send_with_reply_and_block(connection, queryMessage, TIMEOUT, &myError);
    dbus_message_unref(queryMessage);

    if(dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return -1;
    }

    result = extract_i32_from_variant(replyMessage, &myError);
    if(dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return -1;
    }

    dbus_message_unref(replyMessage);
    return result;
}



static int extract_i32_from_variant(DBusMessage *reply, DBusError *error) {
    DBusMessageIter iter;
    DBusMessageIter sub;
    int result ;

    dbus_message_iter_init(reply, &iter);

    if(DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter)){
        dbus_set_error_const(error, "reply_should_be_variant", "this message hasn't a variant type");
        return -1;
    }

    dbus_message_iter_recurse(&iter, &sub);
    if(DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&sub)) {
        dbus_set_error(error, "variant_shoule_be_int32", "the reply message must be int32 content");
        return -1;
    }

    dbus_message_iter_get_basic(&sub, &result);
    return result;
}


static unsigned int get_ui32_property(
    DBusConnection *connection, 
    const char *bus_name, 
    const char *path, 
    const char *iface, 
    const char *propname, 
    DBusError *error
) {
    DBusError myError;
    int result = 0;
    DBusMessage *queryMessage = NULL;
    DBusMessage *replyMessage = NULL;

    dbus_error_init(&myError);
    queryMessage = create_property_get_message(bus_name, path, iface, propname);
    replyMessage  = dbus_connection_send_with_reply_and_block(connection, queryMessage, TIMEOUT, &myError);
    dbus_message_unref(queryMessage);

    if(dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return 0;
    }

    result = extract_ui32_from_variant(replyMessage, &myError);
    if(dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return 0;
    }

    dbus_message_unref(replyMessage);
    return result;
}



static unsigned int extract_ui32_from_variant(DBusMessage *reply, DBusError *error) {
    DBusMessageIter iter;
    DBusMessageIter sub;
    unsigned int result ;

    dbus_message_iter_init(reply, &iter);

    if(DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter)){
        dbus_set_error_const(error, "reply_should_be_variant", "this message hasn't a variant type");
        return 0;
    }

    dbus_message_iter_recurse(&iter, &sub);
    if(DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&sub)) {
        dbus_set_error(error, "variant_shoule_be_uint32", "the reply message must be uint32 content");
        return 0;
    }

    dbus_message_iter_get_basic(&sub, &result);
    return result;
}