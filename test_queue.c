#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

void packet_inspect() {
  int lh, lt;
  lh = 0; lt = 0;
  if (head != NULL) lh = (int)head->len;
  if (tail != NULL) lt = (int)tail->len;
  printf("head: %d\ttail:%d\n", lh, lt);
}

int main() {
  packet_queue *t;
  packet_init();
  char *b1 = malloc(1);
  char *b2 = malloc(2);
  packet_empty();
  packet_inspect();
  printf("ADD 1\n");
  packet_new(b1, 1, NULL);
  packet_inspect();
  printf("ADD 2\n");
  packet_new(b2, 2, NULL);
  packet_inspect();
  printf("POP 1\n");
  t = packet_pop();
  packet_inspect();
  printf("PUSH 1\n");
  packet_push(t);
  packet_inspect();
  printf("POP 2\n");
  t = packet_pop();
  packet_inspect();
  packet_free(t);
  printf("POP 1\n");
  t = packet_pop();
  packet_inspect();
  packet_free(t);
  if (packet_empty()) printf("yay!\n");
  else printf("uhoh..\n");
  return 0;
}