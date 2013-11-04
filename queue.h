#ifndef _QUEUE
#define _QUEUE

#include <stdlib.h>
#include "chunk.h"
#include "udp_utils.h"
#include <stdio.h>

typedef struct packet_queue {
  char *buf;
  size_t len;
  struct sockaddr_in *dest_addr;
  struct packet_queue *next;
} packet_queue;

extern packet_queue *head;
extern packet_queue *tail;

void packet_init();
int packet_empty();
void packet_new(packet *p, struct sockaddr_in *a);
void packet_push(packet_queue *p);
packet_queue *packet_pop();
void packet_free(packet_queue *p);

#endif
