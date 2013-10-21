/* Author: Chris */

#ifndef _WHOHAS
#define _WHOHAS

#include "udp_utils.h"
#include "peer_ihave.h"

/*
 * Calls send_ihave()
 */
int process_whohas(int sock, struct sockaddr_in *from, peer_header *h, void *config);

int send_whohas(int sock, char *chunkfile, void *config);

#endif