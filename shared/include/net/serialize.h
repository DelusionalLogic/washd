#pragma once

#include <unistd.h>

enum MessageType {
	MESSAGE_UNKNOWN = 0,
	MESSAGE_END = 1,
	MESSAGE_DOWN = 2,
};

struct Message {
	size_t size;
	enum MessageType type;
	char payload;
};

void message_read(int socket, struct Message* msg);
void message_write(int socket, struct Message* msg);
size_t message_readsize(int socket);
