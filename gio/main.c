#include <stdio.h>
#include <gio/gio.h>

int main() {
  GError *error = NULL;

  printf("Connecting to session bus...\n");
  GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if(!connection) {
    g_printerr("failed to connect to session bus %s \n", error->message);
    g_error_free(error);
    return 1;
  }
  return 0;
}
