#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include "helpers.h"
#include "general_structures.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[sizeof(struct server_msg) + 1];

	if (argc < 4) {
		usage(argv[0]);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	int fdmax;
	fd_set read_fds;
	struct server_msg* received_msg;
	struct subscriber_msg sent_msg;
	fd_set tmp_fds;
	FD_ZERO(&read_fds);
	FD_SET(0, &read_fds);
	FD_ZERO(&tmp_fds);
	

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
	DIE(ret < 0, "send IP");

	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
	
		if (FD_ISSET(0, &tmp_fds)) {

  		// S-a primit un mesaj.
			memset(buffer, 0, sizeof(struct server_msg) + 1);
			fgets(buffer, sizeof(struct server_msg), stdin);
			// S-a primit comanda exit.
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			// Creez mesajul pe care vreau sa il trimit serverului
			buffer[strlen(buffer) - 1] = 0;

			// Parcurg mesajul primit(comanda de la tastatura) si accesez parametrii
			// pentru a ii pune in structura pe care vreau sa o transmit serverului.
			char *token = strtok(buffer, " ");
			DIE(token == NULL, "comanda gresita 1");

			// Daca primul cuvant incepe cu 's' inseamna ca e "subscribe",
			// altfel e unsubscribe.
			if (token[0] == 's') {
				sent_msg.type = 's';
			} else if (token[0] == 'u') {
				sent_msg.type = 'u';
			} else {
				printf ("comanda gresita 2");
				continue;
			}

			token = strtok(NULL, " ");
			DIE(token == NULL, "comanda gresita 3");
			strcpy(sent_msg.topic, token);

			if (sent_msg.type == 's') {
				token = strtok(NULL, " ");
				DIE(token == NULL, "comanda gresita 4");

				sent_msg.SF = atoi(token);
			}

			// Se trimite mesajul la server.
			n = send(sockfd, (char*) &sent_msg, sizeof(sent_msg), 0);
			DIE(n < 0, "send MESAJ");

			if (sent_msg.type == 's') {
				printf ("subscribed to %s\n", sent_msg.topic);
			} else {
				printf ("unsubscribed from %s\n", sent_msg.topic);
			}
		}

		if (FD_ISSET(sockfd, &tmp_fds))
		{
			memset(buffer, 0, sizeof(server_msg) + 1);
			// Primesc mesajul de la server care contine
			// mesajul de la clientul udp. 
			n = recv(sockfd, buffer, sizeof(server_msg), 0);
			DIE(n < 0, "receive");

			if (n == 0) {
				break;
			}

			// Afisez mesajul de la clientul udp.
			received_msg = (server_msg*) buffer;
			printf ("%s:%u - %s - %s - %s\n", received_msg->ip, received_msg->port,
			 		received_msg->topic, received_msg->type, received_msg->mesaj);
		}
	}

	close(sockfd);

	return 0;
}
