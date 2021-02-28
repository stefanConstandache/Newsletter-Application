#ifndef SUBS_H
#define SUBS_H

#include <algorithm>
#include <cstdint>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <string>
#include <netinet/in.h>
#include <cmath>
#include <zconf.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

using namespace std;

// Structura unui mesaj trimis de la server la un subscriber.
struct server_msg {
    char ip[16];
    uint16_t port;
    char topic[51];
    char type[11];
    char mesaj[1501];
};

// Structura unui mesaj trimis de la un subscriber(client tcp) la un server.
struct subscriber_msg {
    char type;
    char topic[51];
    int SF;
};

// Structura unui client pentru a retine informatiile acestuia.
struct client_info {
    char id[11];
    vector<string> topics;
};

// Structura unui mesaj primit de la un client udp.
struct udp_msg {
    char topic[50];
    uint8_t type;
    char data[1501];
};

#endif