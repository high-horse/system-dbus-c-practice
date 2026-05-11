In the examples that we have seen we talked only about strings but DBus has many datatypes, and they can be basic types and container types.
The basic types are:

- byte (type y);
- boolean (type b);
- int/uint 16/32 bit (type n,q,i,u);
- double (type d);
- unix_fd (type h);
- string (type s);
- object_path (type o);
- signature (type g).


As you have seen every basic type is identified by a type character. This type character is used in the signatures of the methods and of the methods arguments and of the result type. Now we will see the container types:

- struct (type ([field types]), ad ex. a struct composed of two fields, a string and an integer, has type (sn));
- array (type a[element type], ad ex. an array of integers has the type an);
- variant (type v);
- dict (type {[key type][value type]}, ad ex. a dictionary that maps strings to integers has type {sn}).

So, with the exception of the variant type, every container type has a signature composed of two or more characters.
The variant type is strange indeed, because it can represent every valid DBus type and the message will contain the signature of the actual passed value. The variant type is really useful because it enables DBus to manage properties. Here it is the d-feet representation of the org.freedesktop.DBus.Properties interface:

As you see the Get method has the signature:
`Get: (String interface, String propname) -> (Variant value)`


How could you express the signature of the Get method if you wouldn’t have the Variant type?
As you can imagine the Get method let’s you get the value of a property, the GetAll method let’s you get the value of all the properties and the Set method let’s you set the value of a property. Every method takes as argument the name of the interface so a property can be in more than two interface without misunderstandings.

I want to write a simple battery monitor and I want to use the UPower daemon for that. UPower exposes an object for every battery in our computer and I want to use the object at path /org/freedesktop/UPower/devices/battery_BAT1.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
 
static void abort_on_error(DBusError *error);
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
 
    energy = get_double_property(connection, "org.freedesktop.UPower",
                     "/org/freedesktop/UPower/devices/battery_BAT1",
                     "org.freedesktop.UPower.Device",
                     "Energy",
                     &error);
    abort_on_error(&error);
 
    fullEnergy = get_double_property(connection, "org.freedesktop.UPower",
                     "/org/freedesktop/UPower/devices/battery_BAT1",
                     "org.freedesktop.UPower.Device",
                     "EnergyFull",
                     &error);
     
    abort_on_error(&error);
 
    printf("%lf", (energy*100)/fullEnergy);
 
    return 0;
}
 
static void abort_on_error(DBusError *error) {
    if (dbus_error_is_set(error)) {
        fprintf(stderr, "%s", error-&gt;message);
        abort();
    }
}
```

To get the current battery level we want to read the Energy property of the and the EnergyFull as exposed by UPower at the object /org/freedesktop/UPower/devices/battery_BAT1 then we display the battery level percentage. The code gets interesting in the get_double_property:

```c
static double get_double_property(DBusConnection *connection, const char *bus_name, const char *path, const char *iface, const char *propname, DBusError *error) {
    DBusError myError;
    double result = 0;
    DBusMessage *queryMessage = NULL;
    DBusMessage *replyMessage = NULL;
 
    dbus_error_init(&myError);
     
    queryMessage = create_property_get_message(bus_name, path, iface, propname);
    replyMessage = dbus_connection_send_with_reply_and_block(connection,
                          queryMessage,
                          1000,
                          &myError);
    dbus_message_unref(queryMessage);
    if (dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return 0;
    }
 
    result = extract_double_from_variant(replyMessage, &myError);
    if (dbus_error_is_set(&myError)) {
        dbus_move_error(&myError, error);
        return 0;
    }
 
    dbus_message_unref(replyMessage);
     
    return result;
}
```

As you see extracting a property value means calling the Get method of the org.freedesktop.DBus.Properties interface. The value is then extracted from the DBus response with the extract_double_from_variant function. The create_property_get_message just creates a plain DBus method calling message:

```c
static DBusMessage *create_property_get_message(const char *bus_name, const char *path, const char *iface, const char *propname) {
    DBusMessage *queryMessage = NULL;
 
    queryMessage = dbus_message_new_method_call(bus_name, path, 
                            "org.freedesktop.DBus.Properties",
                            "Get");
    dbus_message_append_args(queryMessage,
                 DBUS_TYPE_STRING, &iface,
                 DBUS_TYPE_STRING, &propname,
                 DBUS_TYPE_INVALID);
 
    return queryMessage;
}
```

The extract_double_from_variant function, instead, is really interesting as the double value must be extracted from the variant return type as you would do with a DBus structure:

```c
static double extract_double_from_variant(DBusMessage *reply, DBusError *error) {
    DBusMessageIter iter;
    DBusMessageIter sub;
    double result;
     
    dbus_message_iter_init(reply, &iter);
 
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&iter)) {
        dbus_set_error_const(error, "reply_should_be_variant", "This message hasn't a variant response type");
        return 0;
    }
 
    dbus_message_iter_recurse(&iter, &sub);
 
    if (DBUS_TYPE_DOUBLE != dbus_message_iter_get_arg_type(&sub)) {
        dbus_set_error_const(error, "variant_should_be_double", "This variant reply message must have double content");
        return 0;
    }
 
    dbus_message_iter_get_basic(&sub, &result);
    return result;
}
```

Here we see a new DBus library type: the DBusMessageIter. This type let’s you explore the data attached to a DBus message.

When you initialize an iterator with the dbus_message_iter_init call the iterator is placed on the first element. You can iterate to the next element with dbus_message_iter_next or check with dbus_message_iter_has_next if the iterator has a next element.

You can only extract values for the basic types using the dbus_message_iter_get_basic call. When you need to read a container type, such as our variant type, you need to initialize a sub-iterator with the dbus_message_iter_recurse call. Iterators are stack-only structures and must not be deallocated.

Now you can work with container types.