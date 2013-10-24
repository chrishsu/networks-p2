/* Author: David */

#ifndef _IHAVE
#define _IHAVE

#include "udp_utils.h"

int process_ihave(struct sockaddr_in *from, peer_header *h, void *config);

int send_ihave(int sock, struct sockaddr_in *toaddr, bt_config_t *config, chunk_list *chunks);

#endif
