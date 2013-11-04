/* Author: Chris */

#ifndef _WHOHAS
#define _WHOHAS

#include "udp_utils.h"
#include "peer_ihave.h"
#include "chunk.h"

/*
 * Processes a WHOHAS request then sends an IHAVE packet.
 */
int process_whohas(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends WHOHAS request to all peers.
 */
int send_whohas(int sock, char *chunkfile, bt_config_t *config);

#endif
