#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PACKET_SIZE 56
#define TTL 120

typedef struct ParsedArgs {
    char *address;
    int verbose;
    int usage;
    char *error;
    int ttl;
    float interval;
    int size;
    int timeout;
    int quiet;
} ParsedArgs;

// -- Parse --
ParsedArgs parse_args(int ac, char **av);
int print_usage(ParsedArgs args);

// -- Ping --
char *resolve_hostname(char *hostname);
int init_socket(ParsedArgs args);
unsigned short checksum(void *b, int len);
char *build_packet(ParsedArgs args);

#endif