#pragma once

#include <myhtml/api.h>

#include "vector.h"
#include "types.h"

struct time {
	uint8_t hour;
	uint8_t minute;
};

enum ExtraType {
	ET_CLOSES = 1,
	ET_OPENS,
	ET_REMAINING,
};

struct ParsedStatusExtra {
	enum ExtraType type;
	struct time time;
	char* lastUser;
};

// @NAMING: What is this generic facility thing?
// We'll just replace the pointers when we are done parsing the strings. This
// might be a bit of work for the programmer, but that's not my problem.
struct ElementThing {
	union {
		const char* id_str;
		uint32_t id_num;
	};
	union {
		const char* type_str;
		uint32_t type_num;
	};
	union {
		const char* status_str;
		enum Status status_enum;
	};
	union {
		const char* status_extra_str;
		struct ParsedStatusExtra status_extra_struct;
	};
	union {
		const char* model_str;
	};
	union {
		const char* group_str;
	};
};

void extractElements(myhtml_tree_t* tree, Vector* elementList);

void parseElements(Vector* elements);
