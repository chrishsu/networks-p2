/* Author: David */

#ifndef _IHAVE
#define _IHAVE

#include "udp_utils.h"

int process_ihave(struct sockadrr_in *from, void *config);

int send_ihave(int sock, struct sockaddr_in *to, void *config, chunk_list *chunks);

#endif