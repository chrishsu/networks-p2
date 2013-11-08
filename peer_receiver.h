#ifndef _RECEIVER
#define _RECEIVER


#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "bt_parse.h"
#include "udp_utils.h"
#include "peer_sender.h"
#include "chunk.h"
#include "queue.h"

#define INIT_SEQNUM 1
#define MAX_SEQNUM (INIT_SEQNUM - 1 + BT_CHUNK_SIZE)

#define TIMEOUT_MILLIS (15 * 1000)
#define MAX_TIMEOUTS 5
#define BAD_RESET_MILLIS (30 * 1000)
#define WHOHAS_MILLIS (15 * 1000)

void timeout_check(int sock, bt_config_t *config);
int is_bad(bt_peer_t *peer);
void try_to_get(int sock, bt_chunk_list *chunk, bt_config_t *config);

int send_whohas(int sock, bt_config_t *config);

/**
 * Processes an IHAVE packet, adds to the master chunk_list.
 */
int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends an IHAVE packet.
 */
int send_ihave(int sock, struct sockaddr_in *toaddr, chunk_list *chunks, bt_config_t *config);

/**
 * Sends a GET request for the given hash.
 */
int send_get(int sock, struct sockaddr_in *to, bt_chunk_list *chunk, bt_config_t *config);

/**
 * Adds the data to the master chunk_list, sends an ACK as well.
 */
int process_data(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends an ACK with nextExpected chunk index in the master chunk_list.
 */
int send_ack(int sock, struct sockaddr_in *to, int ack_num);

int add_packet(int sock, bt_chunk_list *chunk, packet *p, bt_config_t *config);
void finish_chunk(int sock, bt_chunk_list *chunk, bt_config_t *config);
bt_chunk_list *chunk_with_id(int id, bt_config_t *config);
void finish_get(bt_config_t *config);
int process_data(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);
void write_to_file(bt_packet_list *packets, FILE *outfile);
void write_to_buffer(bt_packet_list *packets, char *buffer);
void get_all_the_thingz(int sock, bt_config_t *config);

#endif
