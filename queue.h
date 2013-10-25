#include <stdlib.h>

#ifndef _QUEUE
#define _QUEUE

#include <stdio.h>

typedef struct packet_queue {
  char *buf;
  size_t len;
  int idx;
  struct sockaddr *dest_addr;
  struct packet_queue *next;
} packet_queue;

extern packet_queue *head;
extern packet_queue *tail;

void packet_init();
int packet_empty();
void packet_new(char *b, size_t l, struct sockaddr *a);
void packet_push(packet_queue *p);
packet_queue *packet_pop();
void packet_free(packet_queue *p);

#endif
