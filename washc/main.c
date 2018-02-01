#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>

#include "myerror.h"
#include "logger.h"
#include "gdbus/dbus.h"

int main(int argc, char *argv[])
{
	error_init();
	GMainLoop* loop;
	
	loop = g_main_loop_new(NULL, false);

	dbusWashdWasher* washer = dbus_washd_washer_proxy_new_for_bus_sync(
			G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			"dk.slashwin.washd",
			"/dk/slashwin/washd/washers/1",
			NULL,
			NULL
			);

	log_write(LEVEL_INFO, "test %ld", dbus_washd_washer_get_next_time(washer));
	g_main_loop_run(loop);

	g_main_loop_quit(loop);

	return 0;
}
