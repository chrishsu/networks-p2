/* Author: David */

#ifndef _UDP_UTILS
#define _UDP_UTILS

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "debug.h"
#include "chunk.h"

#define TYPE_WHOHAS 0
#define TYPE_IHAVE  1
#define TYPE_GET    2
#define TYPE_DATA   3
#define TYPE_ACK    4
#define TYPE_DENIED 5
#define DROPPED -1

#define MAX_PACKET_SIZE 1500
#define CHUNK_SIZE 20

typedef struct packet_head {
  short magic_num;
  char version;
  char type;
  short header_len;
  short packet_len;
  int seq_num;
  int ack_num;
} packet_head;

typedef struct packet {
  packet_head header;
  char *buf;
} packet;

typedef struct chunk_list {
    char hash[20];
    struct chunk_list *next;
} chunk_list;

long long time_millis();
chunk_list *init_chunk_list();
void free_packet(packet *p);
chunk_list *add_to_chunk_list(chunk_list *list, char *hash);
int chunk_list_len(chunk_list *list);
void del_chunk_list(chunk_list *list);

int hash_equal(char *h1, char *h2);

void init_packet_head(packet_head *h, char type,
                      short header_len, short packet_len,
                      int seq_num, int ack_num);

//void init_peer_header(peer_header *h);
//void free_peer_header(peer_header *h);

/*
 * @returns The type of request, or -1 if invalid type.
 */
//int process_udp(packet *p);

/*
 * Creates p2p header and UDP header, then sends the packet.
 */
//int send_udp(int sock, struct sockaddr_in *toaddr, packet *p, bt_config_t *config);

#endif
