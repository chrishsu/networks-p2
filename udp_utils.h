/* Author: David */

#ifndef _UDP_UTILS
#define _UDP_UTILS

#include <netinet/in.h>

#define TYPE_WHOHAS 0
#define TYPE_IHAVE  1
#define TYPE_GET    2
#define TYPE_DATA   3
#define TYPE_ACK    4
#define TYPE_DENIED 5

#define MAX_PACKET_SIZE 1500
#define CHUNK_SIZE 20

typedef struct {
    short type;
    short pack_len;
    short buf_len;
    int seq_num;
    int ack_num;
    char *buf;
} peer_header;

typedef struct {
  short magic_num;
  char version;
  char type;
  short header_len;
  short packet_len;
  int seq_num;
  int ack_num;
} packet_head;

typedef struct chunk_list {
    char hash[20];
    struct chunk_list *next;
} chunk_list;

chunk_list *init_chunk_list();
chunk_list *add_to_chunk_list(chunk_list *list, char *hash);
int chunk_list_len(chunk_list *list);
void del_chunk_list(chunk_list *list);

void init_peer_header(peer_header *h);
void free_peer_header(peer_header *h);

/*
 * @returns The type of request, or -1 if invalid type.
 */
int process_udp(peer_header *h);

/*
 * Creates p2p header and UDP header, then sends the packet.
 */
int send_udp(int sock, int peer_id, peer_header *h, void *config);

#endif
