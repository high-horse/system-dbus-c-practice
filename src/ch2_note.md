In the previous post we talked about DBUS buses and connection names. In this post we will have a look other basic concepts: object paths and interface names.


When you start d-feet and you select a software “well-known” name, such as org.freedesktop.UPower, you see, on the right side, a list of objects paths.

Every dbus server applications expose objects and every object has a path, which is a slash separated name like `/org/freedesktop/UPower`.

Every object implements one or more interfaces. An interface is a association of methods and properties that another app can call or get/set. Every interface is identified by a name, which is, like the connection name, a reverse domain name separated by dots (ex. `org.freedesktop.DBus.Properties`).

Just to summarize:

- a DBUS server application has a well-known name (ex. org.freedesktop.UPower);
- a DBUS server application exposes one on more objects (ex. /org/freedesktop/UPower, /org/freedesktop/UPower/devices/BAT_1);
- any object implements one or more interfaces (ex. org.freedesktop.DBus.Properties).

You can think of interfaces like the Java or C# ones but in my opinion the most easy way to learn what an interface is to look at d-feet, which has a really good graphical representation of interfaces:


In the following example we will call the `GetCriticalAction` of the `org.freedesktop.UPower` interface exposed by the `/org/freedesktop/UPower` object of the `org.freedesktop.UPower` application. Uff… what a naming!

The `GetCriticalAction` method has the signature:
`GetCriticalAction: () -> (String action)`


That means that this method doesn’t take any arguments and returns a string, which is named action.

```c
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
 
static void check_and_abort(DBusError *error);
 
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
 
    msgQuery = dbus_message_new_method_call(
        interfaceName,
        "/org/freedesktop/UPower",
        "org.freedesktop.UPower",
        "GetCriticalAction");
 
    msgReply = dbus_connection_send_with_reply_and_block(connection, msgQuery, 1000, &error);
    check_and_abort(&error);
    dbus_message_unref(msgQuery);
 
    dbus_message_get_args(msgReply, &error, DBUS_TYPE_STRING, &versionValue, DBUS_TYPE_INVALID);
 
    printf("The critical action is: %s\n", versionValue);
     
    dbus_message_unref(msgReply);
     
    return 0;
}
 
static void check_and_abort(DBusError *error) {
    if (!dbus_error_is_set(error)) return;
    puts(error->message);
    abort();
}
```

Let’s talk about the critical bits. In the previous post you have just seen how to make a connection to the DBus session bus; in this example we are using the system bus. This is the meaning (obvious) of `dbus_bus_get` call.

The `dbus_message_new_method_call` is used to create a new message that must be sent to the bus. The arguments are the coordinates of the process, the object, the interface and the method to call:

```c
msgQuery = dbus_message_new_method_call(
    interfaceName,
    "/org/freedesktop/UPower",
    "org.freedesktop.UPower",
    "GetCriticalAction");
```

When the message is created the function `dbus_connection_send_with_reply_and_block` will send the message on the bus and wait for the response of the other application for one second (the 1000 in argument list). The function returns the reply:

```c
msgReply = dbus_connection_send_with_reply_and_block(connection, msgQuery, 1000, &error);
```

Every DBus message created or returned must be deallocated with the `dbus_message_unref` call:

```c
dbus_message_unref(msgQuery);
```

Now we must extract the result string from the reply message and we do it with `dbus_message_get_args`, which is ok for basic types:

```c
dbus_message_get_args(msgReply, &error, DBUS_TYPE_STRING, &versionValue, DBUS_TYPE_INVALID);
```

Let’s try this code:

```sh
$ ./provadue 
The critical action is: HybridSleep
```

Ok. We have just called a DBUS-exposed object method!

