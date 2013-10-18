/* Author: David */

#ifndef _IHAVE
#define _IHAVE

int process_ihave(struct sockadrr_in *from, void *config);

int send_ihave(struct sockaddr_in *to, void *config);

#endif