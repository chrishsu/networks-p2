#ifndef _SENDER
#define _SENDER

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
#include "chunk.h"
#include "queue.h"

/*
 * Processes a WHOHAS request then sends an IHAVE packet.
 */
int process_whohas(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends an IHAVE packet.
 */
int send_ihave(int sock, struct sockaddr_in *to, chunk_list *chunks, bt_config_t *config);

/**
 * Processes a GET request then sends a DATA packet.
 */
int process_get(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends DATA packet.
 */
int send_data(int sock, struct sockaddr_in *to, char *hash, bt_config_t *config);

/**
 * Processes ACK with connection_list.
 * Main portion of congestion control (slow start & congestion avoidance).
 */
int process_ack(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

void update_window_size(bt_config_t *c, bt_sender_list *sender);

#endif
