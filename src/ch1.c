#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

static void check_and_abort(DBusError *error) {
    if(!dbus_error_is_set(error)) return;

    puts(error->message);
    abort();
}

int main() {
    DBusConnection *connection = NULL;
    DBusError error;
    char buffer[1024];

    dbus_error_init(&error);
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    check_and_abort(&error);

    printf("this is my unique name : %s \n", dbus_bus_get_unique_name(connection));
    fgets(buffer, sizeof(buffer), stdin);

    return 0;
    
}