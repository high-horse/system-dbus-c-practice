### DBus tutorial using the low-level API

In this post we will be studying DBus and how to use it from the so-called Low-level API. The [DBus API documentation](https://dbus.freedesktop.org/doc/api/html/index.html) says “if you use this low-level API directly, you’re signing up for some pain.”


## Basic concepts
First of all some basic concepts. DBus is an IPC mechanism that enable your app to communicate with other apps. You can have two messaging styles: RPC and publisher-subscriber.

In the RPC communication style we have two communicating applications: the client and the server. The client application wants an operation to be executed by the server; this is done with these two steps:

1. The client application send a message to the server requiring the wanted service.
2. The server replies to the client with the service result.

In the publisher-subscriber we have a publisher application and many subscribers. You can think of this communication pattern as event-driven programming. The publisher emits events that all the subscribers will receive.

For two apps to communicate successfully they are to be connected to the same bus. On your system you will have many buses:

- a system bus;
- a session bus for any active session.

The system bus is useful for system-level applications such as PolicyKit, UDisk2, UPower, NetworkManager, DisplayManager and so on.

The session bus is useful for applications of the current sessions such as the XFCE FileManager, PowerManager, SessionManager, etc.

## Connections and names

As you may have already understood DBUS works differently than the TCP communication that you are used to. In the TCP communication style we have the client application to connect to the server one. With DBUS, before applications can communicate each other, they must be connected to the same BUS.

When applications connect to a BUS they receive a unique name which is given by the bus. Let’s try the following C code:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
 
int main() {
    DBusConnection *connection = NULL;
    DBusError error;
    char buffer[1024];
 
    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
        fprintf(stderr, "%s", error.message);
        abort();
    }
 
    puts("This is my unique name");
    puts(dbus_bus_get_unique_name(connection));
    fgets(buffer, sizeof(buffer), stdin);
     
    return 0;
}
```

You need to compile this program using the DBus-1 library like this:

```sh
gcc -o dbus_connection_name -Wall -Wextra dbus_connection_name.c `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1`
```

When you start this program you will see something like this:

```sh
$ ./dbus_connection_name 
This is my unique name
:1.32
```

Now you should install d-feet, which is the [DBUS debugger](https://apps.gnome.org/Dspy/) from the Gnome project, and start it. Point it to the session bus and search, in the left panel, the connection unique name given by the program output. You will see the PID of the application and the command name:

With d-feet you have seen that some applications have a full-blown name instead of the unique name. As an example, if you search in the system bus, you will find applications named `org.freedesktop.NetworkManager`, `org.freedesktop.hostname1`, etc.

An application can request a name, which usually is composed of dot-separated reversed domain name, and the other applications can send messages using that name. This is usually used by server applications.

In this post we have seen some of the basic concept of DBUS and how to create, from a C program, a connection to the session bus. In the next post we will talk about object and interfaces.