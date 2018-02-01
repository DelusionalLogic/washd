#include "parser.h"

#include <myhtml/api.h>
#include <assert.h>
#include <string.h>

#include "myerror.h"
#include "logger.h"

static struct time parseTime(const char* str) {
	assert(str != NULL);
	char* colon = strstr(str, ":");
	assert(colon != NULL);
	struct time t;
	t.hour = atoi(str);
	t.minute = atoi(colon + 1);
	return t;
}

static struct time parseTimeLong(const char* str) {
	assert(str != NULL);
	// @INCOMPLETE: For now we will just assume that long times are always just
	// minutes
	struct time t;
	t.hour = 0;
	t.minute = atoi(str);
	return t;
}

void extractElements(myhtml_tree_t* tree, Vector* elementList) {
	myhtml_collection_t* tables = myhtml_get_nodes_by_tag_id(
			tree,
			NULL,
			MyHTML_TAG_TABLE,
			NULL
			);
	if(tables == NULL) {
		VTHROW_NEW("Failed getting tables");
	}

	myhtml_collection_t* rows = myhtml_get_nodes_by_tag_id_in_scope(
			tree,
			NULL,
			tables->list[1],
			MyHTML_TAG_TR,
			NULL
			);
	myhtml_collection_destroy(tables);
	if(rows == NULL) {
		VTHROW_NEW("Failed finding rows");
	}

	for(size_t row = 2; row < rows->length; row++) {
		myhtml_collection_t* nodes = myhtml_get_nodes_by_tag_id_in_scope(
			tree,
			NULL,
			rows->list[row],
			MyHTML_TAG_TD,
			NULL
		);
		if(nodes == NULL) {
			myhtml_collection_destroy(rows);
			VTHROW_NEW("Failed finding columns");
		}

		// @PERFORMANCE: Arguably we could allocate directly in the vector. It
		// doesn't seems like a big deal right now
		struct ElementThing whiz;
		
		myhtml_tree_node_t* id_node = myhtml_node_child(nodes->list[1]);
		assert(id_node != NULL);
		const char* id_text = myhtml_node_text(id_node, NULL);
		assert(id_text != NULL);
		whiz.id_str = id_text;

		// Group might actually have no text in which case this is NULL
		myhtml_tree_node_t* group_node = myhtml_node_child(nodes->list[4]);
		if(group_node != NULL) {
			const char* group_text = myhtml_node_text(group_node, NULL);
			whiz.group_str = group_text;
		}

		myhtml_tree_node_t* type_node = myhtml_node_child(nodes->list[5]);
		assert(type_node != NULL);
		const char* type_text = myhtml_node_text(type_node, NULL);
		assert(type_text != NULL);
		whiz.type_str = type_text;

		myhtml_tree_node_t* model_node = myhtml_node_child(nodes->list[6]);
		assert(model_node != NULL);
		const char* model_text = myhtml_node_text(model_node, NULL);
		assert(model_text != NULL);
		whiz.model_str = model_text;

		myhtml_tree_node_t* status_node = myhtml_node_child(nodes->list[9]);
		assert(status_node != NULL);
		const char* status_text = myhtml_node_text(status_node, NULL);
		assert(status_text != NULL);
		whiz.status_str = status_text;

		myhtml_tree_attr_t* title_attr = myhtml_attribute_by_key(nodes->list[9], "title", 5);
		assert(title_attr != NULL);
		const char* title_text = myhtml_attribute_value(title_attr, NULL);
		assert(title_text != NULL);
		whiz.status_extra_str = title_text;

		vector_putBack(elementList, &whiz);

		myhtml_collection_destroy(nodes);
	}
	myhtml_collection_destroy(rows);
}

void parseElements(Vector* elements) {
	size_t index;
	struct ElementThing* whiz = vector_getFirst(elements, &index);
	while(whiz != NULL) {
		whiz->id_num = atoi(whiz->id_str);

		whiz->type_num = atoi(whiz->type_str);

		if(strstr(whiz->status_str, "Ledig") == whiz->status_str) {
			whiz->status_enum = STATUS_FREE;
		} else if(strstr(whiz->status_str, "Optaget") == whiz->status_str) {
			whiz->status_enum = STATUS_INUSE;
		}

		if(strstr(whiz->status_extra_str, "Ledig indtil kl. ") == whiz->status_extra_str) {	
			whiz->status_extra_struct.time = parseTime(whiz->status_extra_str + 17);
			whiz->status_extra_struct.type = ET_CLOSES;
			// @INCOMPLETE: We don't parse this yet
			whiz->status_extra_struct.lastUser = "";
		} else if(strstr(whiz->status_extra_str, "Lukker kl. ") == whiz->status_extra_str) {
			whiz->status_extra_struct.time = parseTime(whiz->status_extra_str + 11);
			whiz->status_extra_struct.type = ET_CLOSES;
			// @INCOMPLETE: We don't parse this yet
			whiz->status_extra_struct.lastUser = "";
		} else if(strstr(whiz->status_extra_str, "Lukket indtil kl. ") == whiz->status_extra_str) {
			whiz->status_extra_struct.time = parseTime(whiz->status_extra_str + 18);
			whiz->status_extra_struct.type = ET_OPENS;
			// @INCOMPLETE: We don't parse this yet
			whiz->status_extra_struct.lastUser = "";
		} else if(strstr(whiz->status_extra_str, "Resttid: ") == whiz->status_extra_str) {
			whiz->status_extra_struct.time = parseTimeLong(whiz->status_extra_str + 9);
			whiz->status_extra_struct.type = ET_REMAINING;
			// @INCOMPLETE: We don't parse this yet
			whiz->status_extra_struct.lastUser = "";
		}
		
		// @ENHANCEMENT: Don't handle model and group, what are we going to do
		// about those?

		whiz = vector_getNext(elements, &index);
	}
}
