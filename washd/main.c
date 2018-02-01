#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <myhtml/api.h>
#include <curl/multi.h>

#include "gdbus/dbus.h"
#include "net/serialize.h"
#include "myerror.h"
#include "logger.h"
#include "vector.h"
#include "parser.h"

#define NUM_WASHERS 5
#define NUM_DRYERS 2

struct Event {
	time_t time;
	enum Status status;
};

struct Washer {
	dbusWashdWasher* handle;
	uint32_t id;
	enum Status status;
	struct Event nextEvent;
	char type[64];
};

struct Facility {
	struct Washer washers[NUM_WASHERS];
};

typedef size_t (*ChunkCb)(char* data, size_t size, size_t nmemb, void* userdata);

void requestPage(const char* url, const char* userpwd, ChunkCb cb, void* userdata) {
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_URL, url);
	curl_easy_setopt(eh, CURLOPT_USERPWD, userpwd);
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, userdata);

	curl_easy_perform(eh);
}
struct res_html {
    char  *html;
    size_t size;
};

struct res_html load_html_file(const char* filename)
{
	FILE *fh = fopen(filename, "rb");
	if(fh == NULL) {
		fprintf(stderr, "Can't open html file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	if(fseek(fh, 0L, SEEK_END) != 0) {
		fprintf(stderr, "Can't set position (fseek) in file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	long size = ftell(fh);

	if(fseek(fh, 0L, SEEK_SET) != 0) {
		fprintf(stderr, "Can't set position (fseek) in file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	if(size <= 0) {
		fprintf(stderr, "Can't get file size or file is empty: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char *html = (char*)malloc(size + 1);
	if(html == NULL) {
		fprintf(stderr, "Can't allocate mem for html file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	size_t nread = fread(html, 1, size, fh);
	if (nread != size) {
		fprintf(stderr, "could not read %ld bytes (%ld bytes done)\n", size, nread);
		exit(EXIT_FAILURE);
	}

	fclose(fh);

	struct res_html res = {html, (size_t)size};
	return res;
}

static gboolean on_handle_bar_reload(dbusWashd* obj, GDBusMethodInvocation* inv, gpointer userdata) {
	GMainLoop* dbus = (GMainLoop*)userdata;
	log_write(LEVEL_INFO, "Dbus told us to restart the bar");

	dbus_washd_complete_bar_reload(obj, inv);

	return true;
}

static gboolean on_handle_reload(dbusWashd* obj, GDBusMethodInvocation* inv, gpointer userdata) {
	GMainLoop* dbus = (GMainLoop*)userdata;
	log_write(LEVEL_INFO, "Dbus told us to reload");

	dbus_washd_complete_reload(obj, inv);

	return true;
}

static gboolean on_handle_disable_unit(dbusWashd* obj, GDBusMethodInvocation* inv, char* name, int32_t monitor, gpointer userdata) {
	GMainLoop* dbus = (GMainLoop*)userdata;

	log_write(LEVEL_INFO, "Dbus told us to disable %s, on %d", name, monitor);

	dbus_washd_complete_disable_unit(obj, inv);
return true;
}

static gboolean on_handle_enable_unit(dbusWashd* obj, GDBusMethodInvocation* inv, char* name, int32_t monitor, gpointer userdata) {
	GMainLoop* dbus = (GMainLoop*)userdata;

	log_write(LEVEL_INFO, "Dbus told us to enable %s, on %d", name, monitor);

	dbus_washd_complete_enable_unit(obj, inv);

	return true;
}

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	Vector* str = (Vector*)userdata;
	vector_putListBack(str, ptr, size*nmemb);
	return size*nmemb;
}

static int testfun(gpointer userdata) {
	struct Facility* facility = (struct Facility*)userdata;
	log_write(LEVEL_INFO, "TIMES UP!");
	char str[16];
	sprintf(str, "%d", rand());

	struct res_html res = load_html_file("/home/delusional/Downloads/MieleLogic - Serie:8201 Nr:11 - Damstræde 106 - 9220 Aalborg Ø2.html");

	myhtml_t* myhtml = myhtml_create();
	myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);

	myhtml_tree_t* tree = myhtml_tree_create();
	myhtml_tree_init(tree, myhtml);

	/* 
	Vector html;
	vector_init(&html, sizeof(char), 512);

			write_cb, &html);

	vector_putBack(&html, "\0");
	myhtml_parse(tree, MyENCODING_UTF_8, html.data, vector_size(&html));
	// log_write(LEVEL_INFO, "%s", html.data);
	vector_kill(&html);
	/*/
	myhtml_parse(tree, MyENCODING_UTF_8, res.html, res.size);
	//*/
	
	Vector elements;
	vector_init(&elements, sizeof(struct ElementThing), 13);

	extractElements(tree, &elements);
	parseElements(&elements);

	assert(vector_size(&elements) == 11);

	size_t index;
	struct ElementThing* whiz = vector_getFirst(&elements, &index);
	while(whiz != NULL) {
		if(whiz->type_num == 51) {
			assert(whiz->id_num - 1 < NUM_WASHERS);
			// @INCOMPLETE: Assume that the ids are contiguous and start at 1
			struct Washer* washer = &facility->washers[whiz->id_num-1];

			washer->id = whiz->id_num;
			dbus_washd_washer_set_id(washer->handle, washer->id);

			if(whiz->status_enum == STATUS_FREE && whiz->status_extra_struct.type == ET_OPENS) {
				washer->status = STATUS_CLOSED;
				dbus_washd_washer_set_status(washer->handle, washer->status);
			} else if(whiz->status_enum == STATUS_FREE) {
				washer->status = STATUS_FREE;
				dbus_washd_washer_set_status(washer->handle, washer->status);
			} else if(whiz->status_enum == STATUS_INUSE) {
				washer->status = STATUS_INUSE;
				dbus_washd_washer_set_status(washer->handle, washer->status);
			}
			assert(washer->status != 0);

			if(washer->status == STATUS_FREE) {
				if(whiz->status_extra_struct.type == ET_CLOSES) {
					time_t curTime = time(NULL);
					struct tm* tm = localtime(&curTime);
					assert(tm != NULL);

					tm->tm_hour = whiz->status_extra_struct.time.hour;
					tm->tm_min = whiz->status_extra_struct.time.minute;

					washer->nextEvent.time = mktime(tm);
					washer->nextEvent.status = STATUS_CLOSED;
					dbus_washd_washer_set_next_status(washer->handle, washer->nextEvent.status);
					dbus_washd_washer_set_next_time(washer->handle, washer->nextEvent.time);
				}
			} else if(washer->status == STATUS_CLOSED) {
				if(whiz->status_extra_struct.type == ET_OPENS) {
					time_t curTime = time(NULL);
					struct tm* tm = localtime(&curTime);
					assert(tm != NULL);

					tm->tm_hour = whiz->status_extra_struct.time.hour;
					tm->tm_min = whiz->status_extra_struct.time.minute;

					washer->nextEvent.time = mktime(tm);
					washer->nextEvent.status = STATUS_FREE;
					dbus_washd_washer_set_next_status(washer->handle, washer->nextEvent.status);
					dbus_washd_washer_set_next_time(washer->handle, washer->nextEvent.time);
				}
			}else if(washer->status == STATUS_INUSE) {
				if(whiz->status_extra_struct.type == ET_REMAINING) {
					time_t curTime = time(NULL);
					struct tm* tm = localtime(&curTime);
					assert(tm != NULL);

					tm->tm_hour += whiz->status_extra_struct.time.hour;
					tm->tm_min += whiz->status_extra_struct.time.minute;

					washer->nextEvent.time = mktime(tm);
					washer->nextEvent.status = STATUS_FREE;
					dbus_washd_washer_set_next_status(washer->handle, washer->nextEvent.status);
					dbus_washd_washer_set_next_time(washer->handle, washer->nextEvent.time);
				}
			}
		}

		whiz = vector_getNext(&elements, &index);
	}

	myhtml_tree_destroy(tree);
	myhtml_destroy(myhtml);
	free(res.html);

	return true;
}

static void on_bus_aquired(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	struct Facility* facility = (struct Facility*)userdata;
	dbusWashd* washdbus = dbus_washd_skeleton_new();
	GError* error = NULL;
	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (washdbus),
				conn,
				"/dk/slashwin/washd",
				&error))
	{
		log_write(LEVEL_ERROR, "Failed exporting skeleton: %s", error->message);
		g_error_free(error);
		return;
	}
	g_signal_connect(G_DBUS_INTERFACE_SKELETON(washdbus),
			"handle-bar-reload",
			G_CALLBACK(on_handle_bar_reload),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(washdbus),
			"handle-reload",
			G_CALLBACK(on_handle_reload),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(washdbus),
			"handle-disable-unit",
			G_CALLBACK(on_handle_disable_unit),
			userdata);

	g_signal_connect(G_DBUS_INTERFACE_SKELETON(washdbus),
			"handle-enable-unit",
			G_CALLBACK(on_handle_enable_unit),
			userdata);

	// Create the objects now to let the parser write to them
	for(size_t i = 0; i < NUM_WASHERS; i++) {
		struct Washer* washer = &facility->washers[i];
		washer->handle = dbus_washd_washer_skeleton_new();
	}

	testfun(userdata);

	Vector paths={0};
	vector_init(&paths, sizeof(char*), NUM_WASHERS);
	ERROR_ABORT("While initializing washer object list");

	for(size_t i = 0; i < NUM_WASHERS; i++) {
		struct Washer* washer = &facility->washers[i];

		char* buff = malloc(64 * sizeof(char));
		snprintf(buff, 64, "/dk/slashwin/washd/washers/%d", washer->id);


		log_write(LEVEL_INFO, "Exporting %d to %s", washer->id, buff);
		int status = g_dbus_interface_skeleton_export(
				G_DBUS_INTERFACE_SKELETON(washer->handle),
				conn,
				buff,
				&error);
		if (!status) {
			ERROR_NEW("Failed exporting skeleton %s", error->message);
		}
		vector_putBack(&paths, &buff);
		ERROR_ABORT("While building washer object list");
		buff = NULL;
	}
	char* null = NULL;
	vector_putBack(&paths, &null);
	ERROR_ABORT("While appending final NULL to washer object list");

	dbus_washd_set_washers(washdbus, (const char* const*)vector_detach(&paths));

	/* guint timer_id = */
	g_timeout_add_seconds(
		10,
		testfun,
		userdata
	);
}

static void on_name_aquired(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	log_write(LEVEL_INFO, "Now taking calls on the dbus");
}

static void on_name_lost(GDBusConnection* conn, const gchar* name, gpointer userdata) {
	log_write(LEVEL_ERROR, "Lost dbus name");
}

int main(int argc, char **argv) {
	error_init();
	GMainLoop* loop;
	struct Facility facility = {0};
	
	loop = g_main_loop_new(NULL, false);

	guint owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
			"dk.slashwin.washd",
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
			on_bus_aquired,
			on_name_aquired,
			on_name_lost,
			&facility,
			NULL);

	g_main_loop_run(loop);
	g_bus_unown_name(owner_id);

	g_main_loop_quit(loop);

	return 0;
}
