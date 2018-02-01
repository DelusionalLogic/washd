#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

#include "myerror.h"
#include "types.h"
#include "vector.h"
#include "logger.h"
#include "gdbus/dbus.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int main(int argc, char *argv[])
{
	error_init();
	GMainLoop* loop;
	
	loop = g_main_loop_new(NULL, false);

	dbusWashd* root = dbus_washd_proxy_new_for_bus_sync(
			G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			"dk.slashwin.washd",
			"/dk/slashwin/washd",
			NULL,
			NULL);
	assert(root != NULL);

	char** washer_paths = dbus_washd_get_washers(root);

	Vector washers;
	vector_init(&washers, sizeof(dbusWashdWasher*), 5);

	size_t index = 0;
	while(washer_paths[index] != NULL) {
		char* washer_path = washer_paths[index];

		dbusWashdWasher* washer = dbus_washd_washer_proxy_new_for_bus_sync(
				G_BUS_TYPE_SESSION,
				G_DBUS_PROXY_FLAGS_NONE,
				"dk.slashwin.washd",
				washer_path,
				NULL,
				NULL);
		assert(washer != NULL);

		vector_putBack(&washers, &washer);
		index++;
	}

	dbusWashdWasher** washer_ptr = vector_getFirst(&washers, &index);
	while(washer_ptr != NULL) {
		dbusWashdWasher* washer = *washer_ptr;

		int id = dbus_washd_washer_get_id(washer);
		int status = dbus_washd_washer_get_status(washer);

		// @CLEANUP: Extract to method (colorForStatus(status))
		char* color;
		switch(status) {
			case STATUS_FREE:
				color = ANSI_COLOR_GREEN;
				break;
			case STATUS_CLOSED:
				color = ANSI_COLOR_RED;
				break;
			case STATUS_INUSE:
				color = ANSI_COLOR_RED;
				break;
			default:
				assert(false);
		}

		printf("Machine %d ......... %s%s" ANSI_COLOR_RESET "\n", id, color, STATUS_STR[status]);

		washer_ptr = vector_getNext(&washers, &index);
	}
	return 0;
}
