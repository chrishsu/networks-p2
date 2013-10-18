/* Author: Chris */

#ifndef _WHOHAS
#define _WHOHAS

int process_whohas(struct sockaddr_in *from, void *config);

int send_whohas(char *chunkfile, void *config);

#endif