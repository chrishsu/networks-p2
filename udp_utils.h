/* Author: David */

#ifndef _UDP_UTILS
#define _UDP_UTILS

#define TYPE_WHOHAS 0
#define TYPE_IHAVE  1
#define TYPE_GET    2
#define TYPE_DATA   3
#define TYPE_ACK    4
#define TYPE_DENIED 5

typedef struct {
    short type;
    short buf_len;
    int seq_num;
    int ack_num;
    char *buf;
} peer_header;

typedef struct chunk_list {
    char hash[20];
    struct chunk_list *next;
} chunk_list;

/*
 * @returns The type of request, or -1 if invalid type.
 */
int process_udp(char *buf);

/*
 * Creates p2p header and UDP header, then sends the packet.
 */
int send_udp(int sock, peer_header *h);

#endif