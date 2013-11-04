/* Author: David */

#ifndef _IHAVE
#define _IHAVE

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "udp_utils.h"
#include "queue.h"
#include "peer_whohas.h"

/**
 * Processes an IHAVE packet, adds to the master chunk_list.
 */
int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends an IHAVE packet.
 */
int send_ihave(int sock, struct sockaddr_in *toaddr, bt_config_t *config, chunk_list *chunks);

#endif
