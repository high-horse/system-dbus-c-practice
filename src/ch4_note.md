### DBus tutorial, a simple server

Many DBus objects implement the `org.freedesktop.DBus.Introspectable` interface. This interface lets other apps know which interfaces does an object implement and how these interfaces are composed. The `org.freedesktop.DBus.Introspectable` interface has only one simple method:

`org.freedesktop.DBus.Introspectable.Introspect (out STRING xml_data)`


The Introspect method, called on an object, returns an XML description of the interfaces implemented. This is an example:

```code
<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="data" direction="out" type="s"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg name="interface" direction="in" type="s"/>
      <arg name="propname" direction="in" type="s"/>
      <arg name="value" direction="out" type="v"/>
    </method>
    <method name="Set">
      <arg name="interface" direction="in" type="s"/>
      <arg name="propname" direction="in" type="s"/>
      <arg name="value" direction="in" type="v"/>
    </method>
    <method name="GetAll">
      <arg name="interface" direction="in" type="s"/>
      <arg name="props" direction="out" type="a{sv}"/>
    </method>
  </interface>
  <interface name="org.ekiga.Ekiga">
    <method name="GetUserComment">
      <arg name="arg0" type="s" direction="out"/>
    </method>
    <method name="GetUserLocation">
      <arg name="arg0" type="s" direction="out"/>
    </method>
    <method name="GetUserName">
      <arg name="arg0" type="s" direction="out"/>
    </method>
    <method name="Call">
      <arg name="uri" type="s" direction="in"/>
    </method>
    <method name="Shutdown">
    </method>
    <method name="Show">
    </method>
  </interface>
</node>
```

The XML given from DBus instrospection is really straightforward. The are collection of interfaces and every interfaces has methods. For every method every argument is announced with his direction and his type.

### Code

Every application connected to a DBus bus gets a unique name. If the application want to be reached by other application it must get a “well-known” name. To get a well-known name we want to use the `dbus_bus_request_name`.


The `dbus_connection_try_register_object_path` tries to register a new object for the current application. The object path is specified in the second argument and the third argument is a coupe of callback functions, in which I use only the `message_functions` there are called when a message directing to our object is received on the bus. We will see the tutorial_messages function later.


The next new function is `dbus_connection_read_write_dispatch` which reads a message from the DBus queue, blocking until a message is available, and dispatch the message to the interested object.

```c
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check_and_abort(DBusError *error);
static DBusHandlerResult tutorial_messages(DBusConnection *connection, DBusMessage *message, void *user_data);
static void respond_to_introspect(DBusConnection *connection, DBusMessage *request);
static void respond_to_sum(DBusConnection *connection, DBusMessage *request);

int main() {
	DBusConnection *connection;
	DBusError error;
	DBusObjectPathVTable vtable;

	dbus_error_init(&error);
	connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	check_and_abort(&error);

	dbus_bus_request_name(connection, "it.interfree.leonardoce.DBusTutorial", 0, &error);
	check_and_abort(&error);

	vtable.message_function = tutorial_messages;
	vtable.unregister_function = NULL;
	
	dbus_connection_try_register_object_path(connection,
						 "/it/interfree/leonardoce/DBusTutorial",
						 &vtable,
						 NULL,
						 &error);
	check_and_abort(&error);

	while(1) {
		dbus_connection_read_write_dispatch(connection, 1000);
	}
	
	return 0;
}
```

This is our classical error checking function:

```c
static void check_and_abort(DBusError *error) {
	if (dbus_error_is_set(error)) {
		puts(error->message);
		abort();
	}
}
```

This is the heart of our server. This function reads the message using `dbus_message_get_interface` and dbus_message_get_member and then routes the message to the appropriate function. If this function returns `“DBUS_HANDLER_RESULT_HANDLED”` then the DBus library doesn’t consider other handlers, instead if the functions returns `“DBUS_HANDLER_RESULT_NOT_YET_HANDLED”` then the library will process other handlers.


```c
static DBusHandlerResult tutorial_messages(DBusConnection *connection, DBusMessage *message, void *user_data) {
	const char *interface_name = dbus_message_get_interface(message);
	const char *member_name = dbus_message_get_member(message);
	
	if (0==strcmp("org.freedesktop.DBus.Introspectable", interface_name) &&
	    0==strcmp("Introspect", member_name)) {

		respond_to_introspect(connection, message);
		return DBUS_HANDLER_RESULT_HANDLED;
	} else if (0==strcmp("it.interfree.leonardoce.DBusTutorial", interface_name) &&
		   0==strcmp("Sum", member_name)) {
		
		respond_to_sum(connection, message);
		return DBUS_HANDLER_RESULT_HANDLED;
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
}
```

Here is the functions who responds to the introspection calls. We only respond with the XML data:

```c
static void respond_to_introspect(DBusConnection *connection, DBusMessage *request) {
	DBusMessage *reply;

	const char *introspection_data =
		" <!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
		"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
		" <!-- dbus-sharp 0.8.1 -->"
		" <node>"
		"   <interface name=\"org.freedesktop.DBus.Introspectable\">"
		"     <method name=\"Introspect\">"
		"       <arg name=\"data\" direction=\"out\" type=\"s\" />"
		"     </method>"
		"   </interface>"
		"   <interface name=\"it.interfree.leonardoce.DBusTutorial\">"
		"     <method name=\"Sum\">"
		"       <arg name=\"a\" direction=\"in\" type=\"i\" />"
		"       <arg name=\"b\" direction=\"in\" type=\"i\" />"
		"       <arg name=\"ret\" direction=\"out\" type=\"i\" />"
		"     </method>"
		"   </interface>"
		" </node>";
	
	reply = dbus_message_new_method_return(request);
	dbus_message_append_args(reply,
				 DBUS_TYPE_STRING, &introspection_data,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(connection, reply, NULL);
	dbus_message_unref(reply);
}
```

Our sample object has a `“Sum”` call which we implement here:

```c
static void respond_to_sum(DBusConnection *connection, DBusMessage *request) {
	DBusMessage *reply;
	DBusError error;
	int a=0, b=0, ret=0;

	dbus_error_init(&error);

	dbus_message_get_args(request, &error,
			      DBUS_TYPE_INT32, &a,
			      DBUS_TYPE_INT32, &b,
			      DBUS_TYPE_INVALID);
	if (dbus_error_is_set(&error)) {
		reply = dbus_message_new_error(request, "wrong_arguments", "Illegal arguments to Sum");
		dbus_connection_send(connection, reply, NULL);
		dbus_message_unref(reply);
		return;
	}

	ret = a+b;
	
	reply = dbus_message_new_method_return(request);
	dbus_message_append_args(reply,
				 DBUS_TYPE_INT32, &ret,
				 DBUS_TYPE_INVALID);
	dbus_connection_send(connection, reply, NULL);
	dbus_message_unref(reply);
}
```

We use the `dbus_message_get_args` to read the incoming messages and the `dbus_message_new_method_return` to create a reply message based on the query message. The `dbus_connection_send` function gets the message in the output
queue.

This is our first DBus server! You can test it with d-feel double clicking on the Sum method and giving two number as 32,33. It will answer with the correct sum.


#### Usage/Test

```sh
# intall with sudo apt install libglib2.0-bin
# introspect
gdbus introspect \
  --session \
  --dest it.interface.camel.DBusTutorial \
  --object-path /it/interface/camel/DBusTutorial

# method call "sum"
gdbus call \
  --session \
  --dest it.interface.camel.DBusTutorial \
  --object-path /it/interface/camel/DBusTutorial \
  --method it.interface.camel.DBusTutorial.Sum \
  10 20


# using dbus-send
dbus-send \
  --session \
  --dest=it.interface.camel.DBusTutorial \
  --print-reply \
  /it/interface/camel/DBusTutorial \
  it.interface.camel.DBusTutorial.Sum \
  int32:10 int32:20
```