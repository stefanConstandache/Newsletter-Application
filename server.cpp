#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <algorithm>
#include <netinet/tcp.h>
#include "helpers.h"
#include "general_structures.h"

#define MAXBUFLEN 2000

using namespace std;

struct server_msg mesaj_udp(struct udp_msg* cli_udp_msg) {

	struct server_msg new_cli_tcp_msg;

	strcpy(new_cli_tcp_msg.topic, cli_udp_msg->topic);
	new_cli_tcp_msg.topic[50] = 0;

	if (cli_udp_msg->type == 0) {
		int val = *(uint32_t*) (cli_udp_msg->data + 1);
		val = ntohl(val);
		if (cli_udp_msg->data[0] != 0) {
			val = -val;
		}

		sprintf(new_cli_tcp_msg.mesaj, "%d", val);
		strcpy(new_cli_tcp_msg.type, "INT");
	} else if (cli_udp_msg->type == 1) {

		float val = *(uint16_t*) (cli_udp_msg->data);
		val = ntohs(val);
		val /= 100;

		sprintf(new_cli_tcp_msg.mesaj, "%.2f", val);
		strcpy(new_cli_tcp_msg.type, "SHORT_REAL");
	} else if (cli_udp_msg->type == 2) {

		float val = *(uint32_t*) (cli_udp_msg->data + 1);
		val = ntohl(val);
		val /= pow(10, cli_udp_msg->data[5]);

		if (cli_udp_msg->data[0] != 0) {
			val = -val;
		}

		sprintf(new_cli_tcp_msg.mesaj, "%lf", val);
		strcpy(new_cli_tcp_msg.type, "FLOAT");
	} else if (cli_udp_msg->type == 3) {

		strcpy(new_cli_tcp_msg.mesaj, cli_udp_msg->data);
		strcpy(new_cli_tcp_msg.type, "STRING");
	}

	return new_cli_tcp_msg;
}

int main(int argc, char *argv[])
{
	DIE (argc < 2, "Usage: %s server_port\n");

	// Vector pentru retinerea informatiilor clientilor dupa file descriptor.
	// Informatia unui client consta in id-ul acestuia si topicurile la care
	// este abonat.
	vector<pair<int, struct client_info>> clienti;

	// Vector pentru retinerea topicurilor impreuna cu file descriptorii
	// clientilor care sunt abonati la topicul respectiv.
	vector<pair<string, vector<int>>> topic_fd;

	int sock_tcp, sock_udp, newsockfd, portno;
	char buffer[MAXBUFLEN];
	struct sockaddr_in serv_addr, cli_tcp, cli_udp;
	int n, i, ret;
	socklen_t socklen_tcp = sizeof(sockaddr);
	socklen_t socklen_udp = sizeof(sockaddr);

	struct client_info client_info; 
	struct udp_msg* cli_udp_msg;
	struct server_msg cli_tcp_msg; 
	struct subscriber_msg* subs_msg;

	// multimea de citire folosita in select()
	fd_set read_fds;	

	// multime folosita temporar
	fd_set tmp_fds;		

	// valoare maxima fd din multimea read_fds
	int fdmax;			

	
	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "socket tcp");

	sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "socket udp");

	portno = atoi(argv[1]);
	DIE(portno < 1024, "atoi");

	cli_udp.sin_family = AF_INET;
	cli_udp.sin_port = htons(portno);
	cli_udp.sin_addr.s_addr = INADDR_ANY;
	cli_tcp.sin_family = AF_INET;
	cli_tcp.sin_port = htons(portno);
	cli_tcp.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_tcp, (sockaddr *) &cli_tcp, sizeof(sockaddr_in));
	DIE(ret < 0, "bind_tcp");

	ret = bind(sock_udp, (sockaddr *) &cli_udp, sizeof(sockaddr_in));
	DIE(ret < 0, "bind_udp");

	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sock_tcp;

	while (1) {
		int ok = 0;
		tmp_fds = read_fds; 
		memset(buffer, 0, strlen(buffer));

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE (ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {

					fgets(buffer, sizeof(buffer) - 1, stdin);
					
					if (strcmp(buffer, "exit\n") == 0) {
						ok = 1;
						break;
					}
				} else if (i == sock_tcp) {
					newsockfd = accept(sock_tcp, (struct sockaddr *) &cli_tcp, &socklen_tcp);
					DIE(newsockfd < 0, "accept");
					
					// Dezactiveaza algoritmul lui Nagle.
					int flag = 1;
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

					// Se adauga noul socket intors de accept() 
					// la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					ret = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);
					DIE(ret < 0, "recv tcp");

					// Copiez in structura de client id-ul pentru a il afisa la logare.
					strcpy(client_info.id, buffer);

					// Adaug noul client la vectorul de clienti.
					pair<int, struct client_info> p;
					p.first = newsockfd;
					p.second = client_info;
					clienti.push_back(p);

					printf ("New client %s connected from %s:%d\n",
					buffer, inet_ntoa(cli_tcp.sin_addr), ntohs(cli_tcp.sin_port));

				} else if (i == sock_udp) {
					ret = recvfrom(sock_udp, buffer, sizeof(buffer) - 1, 0, 
							(sockaddr*) &cli_udp, &socklen_udp);
					DIE(ret < 0, "recv udp");

					cli_udp_msg = (udp_msg*) buffer;

					// Transform mesajul primit de la clientul udp
					// pentru a putea fi citit de clientul tcp(subscriber-ul)
					cli_tcp_msg = mesaj_udp(cli_udp_msg);

					// Pun ip-ul si portul clientului udp in mesajul
					// pe care vreau sa il transmit subscriberilor.
					cli_tcp_msg.port = ntohs(cli_udp.sin_port);
					char* buff = inet_ntoa(cli_udp.sin_addr);
					strcpy(cli_tcp_msg.ip, buff);

					// Transmit mesajul subscriberilor care sunt abonati la topicul respectiv.
					int j;
					for (j = 0; j < topic_fd.size(); j++) {
						pair<string, vector<int>> p = topic_fd[j];

						if (p.first == cli_tcp_msg.topic) {

							int k;
							for (k = 0; k < p.second.size(); k++) {
								int fd = p.second[k];
								ret = send(fd, (char*) &cli_tcp_msg, sizeof(server_msg), 0);
								DIE(ret < 0, "send in udp");
							}

							break;
						}
					}

				} else {
					// S-au primit date pe unul din socketii de client
					n = recv(i, buffer, sizeof(buffer) - 1, 0);
					DIE(n < 0, "recv");
					int u;

					if (n == 0) {
						// Conexiunea s-a inchis
						int k;
						for (k = 0; k < clienti.size(); k++) {
							if (clienti[k].first == i) {
								printf("Client %s disconnected\n", clienti[k].second.id);
								break;
							}
						}

						// Se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);

						int j;
						for (j = fdmax; j > 2; j--) {
							if (FD_ISSET(j, &read_fds)) {
								fdmax = j;
								break;
							}
						}

						close(i);
					} else {
						subs_msg = (subscriber_msg*) buffer;

						if (subs_msg->type == 'u') {
							// Verific daca clientul exista si e abonat la topic.
							// Daca este atunci il sterg din vectorul de clienti
							int j;
							for (j = 0; j < topic_fd.size(); j++) {
								if (topic_fd[j].first == subs_msg->topic) {
									int k;
									for (k = 0; k < topic_fd[j].second.size(); k++) {
										if (topic_fd[j].second[k] == i) {
											int r;
											for (r = 0; r < clienti.size(); r++) {
												if (clienti[r].first == i) {
													int l;
													for (l = 0; l < clienti[r].second.topics.size(); l++) {
														if (clienti[r].second.topics[l] == subs_msg->topic) {
															clienti[r].second.topics.erase(clienti[r].second.topics.begin() + l);
														
														break;
														}
													}

													break;
												}
											}

											topic_fd[j].second.erase(topic_fd[j].second.begin() + k);
											break;
										}
									}

									break;
								}
							}

						} else if (subs_msg->type == 's') {
							// Verific daca exita deja topicul la care
							// vrea sa se aboneze clientul.
							// Daca exita atunci adaug file descriptorul clientului in vector.
							int j;
							int inexistent = 1;
							for (j = 0; j < topic_fd.size(); j++) {
								if (topic_fd[j].first == subs_msg->topic) {
									topic_fd[j].second.push_back(i);
									inexistent = 0;
									break;
								}
							}

							// Daca topicul nu exista se creaza
							// un topic cu numele respectiv.
							if (inexistent) {
								pair<string, vector<int>> p;
								p.first = subs_msg->topic;
								p.second.push_back(i);
								topic_fd.push_back(p);
							}

							// Se adauga topicul si la clientul respectiv.
							string s;
							s = subs_msg->topic;
							int k;
							for (k = 0; k < clienti.size(); k++) {
								if (clienti[k].first == i) {
									clienti[k].second.topics.push_back(s);
								}
							}
						}
					}
				}
			}
		}

		// Daca s-a primit comanda "exit" atunci ies din ciclul "while".
		if (ok == 1) {
			break;
		}
	}

	// Inchid toti sochetii care sunt folositi.
	for (i = 2; i <= fdmax; i++) {
		if (FD_ISSET(i, &read_fds)) {
			close(i);
		}
	}

	return 0;
}
