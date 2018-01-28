#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BUFFER_SIZE 12

int main(int argc, char **argv) {
	printf("BEEP BOOP\n");
	int connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "socket", sizeof(addr.sun_path)-1);

	int ret;
	ret = bind(connection_socket, (const struct sockaddr *)&addr,
			sizeof(struct sockaddr_un));
	if (ret == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	printf("BIND\n");

	ret = listen(connection_socket, 20);
	if (ret == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* This is the main loop for handling connections. */
	int down_flag = 0;
	char buffer[BUFFER_SIZE];
	int result;
	int data_socket;
	for (;;) {

		/* Wait for incoming connection. */

		data_socket = accept(connection_socket, NULL, NULL);
		if (data_socket == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("NEW CONNECTION\n");

		result = 0;
		for(;;) {

			/* Wait for next data packet. */

			ret = read(data_socket, buffer, BUFFER_SIZE);
			if (ret == -1) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			/* Ensure buffer is 0-terminated. */

			buffer[BUFFER_SIZE - 1] = 0;
			
			printf("RECV %s\n", buffer);

			/* Handle commands. */

			if (!strncmp(buffer, "DOWN", BUFFER_SIZE)) {
				down_flag = 1;
				break;
			}

			if (!strncmp(buffer, "END", BUFFER_SIZE)) {
				break;
			}

			/* Add received summand. */

			result += atoi(buffer);
		}

		/* Send result. */

		sprintf(buffer, "%d", result);
		ret = write(data_socket, buffer, BUFFER_SIZE);

		if (ret == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		/* Close socket. */

		close(data_socket);

		/* Quit on DOWN command. */

		if (down_flag) {
			break;
		}
	}

	close(connection_socket);

	/* Unlink the socket. */

	unlink("socket");

	exit(EXIT_SUCCESS);

	printf("Hello there.\n");
	return 0;
}
