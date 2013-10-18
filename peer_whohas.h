/* Author: Chris */

#ifndef _WHOHAS
#define _WHOHAS

#include "udp_utils.h"

int process_whohas(int sock, struct sockaddr_in *from, void *config);

int send_whohas(int sock, char *chunkfile, void *config);

#endif