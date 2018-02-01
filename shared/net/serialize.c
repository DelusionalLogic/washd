#include "net/serialize.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>

#include "logger.h"
#include "myerror.h"

#define BUFFER_SIZE 12

size_t message_readsize(int socket) {
	// @INCOMPLETE: Who cares about endianess anyway
	size_t msg_size;
	if(recv(socket, &msg_size, sizeof(msg_size), MSG_PEEK) == -1)
		THROW_NEW(-1, "Failed peeking the size of the packet");
	return msg_size;
}

void message_read(int socket, struct Message* msg) {
	size_t msg_size = message_readsize(socket);
	log_write(LEVEL_INFO, "Got size %ld", msg_size);

	if(recv(socket, msg, msg_size, 0) == -1) {
		log_write(LEVEL_WARNING, "Error reading from socket");
		VTHROW_NEW("Failed reading packet from socket");
	}
}

void message_write(int socket, struct Message* msg) {
	log_write(LEVEL_INFO, "Sending size %ld", msg->size);

	if(send(socket, msg, msg->size, 0) == -1) {
		log_write(LEVEL_WARNING, "Error writing to socket");
		VTHROW_NEW("Failed writing packet to socket");
	}
}
