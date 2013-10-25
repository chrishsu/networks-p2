#include "queue.h"

packet_queue *head;
packet_queue *tail;
int PQindex;

/**
 * Initializes the packet_queue.
 */
void packet_init() {
  head = NULL;
  tail = NULL;
  PQindex = 0;
}

/**
 * Is the queue empty?
 *
 * @return 1 if it is, 0 otherwise.
 */
int packet_empty() {
  if (head == NULL && tail == NULL) return 1;
  return 0;
}

/**
 * Creates a new packet and adds it to the queue.
 *
 * @param[in] b
 *    The buffer.
 * @param[in] l
 *    The length of the buffer.
 * @param[in] a
 *    The sockaddr struct.
 */
void packet_new(char *b, size_t l, struct sockaddr *a) {
  packet_queue *n = malloc(sizeof(packet_queue));
  PQindex++;
  n->idx = PQindex;
  n->buf = b;
  n->len = l;
  n->dest_addr = a;
  n->next = NULL;
  packet_push(n);
}

void packet_inspect() {
  int lh, lt;
  lh = 0; lt = 0;
  if (head != NULL) lh = (int)head->idx;
  if (tail != NULL) lt = (int)tail->idx;
  printf("head: %d\ttail:%d\n", lh, lt);
}

/**
 * Adds a packet to the queue.
 *
 * @param[in] p
 *    The packet.
 */
void packet_push(packet_queue *p) {
  if (p == NULL) return;
  if (packet_empty()) {
    head = p;
    tail = p;
  }
  else {
    tail->next = p;
    tail = p;
  }
  //printf("index: %d\n", p->idx);
  //packet_inspect();
}

/**
 * Returns the next packet.
 *
 * @return The next packet or NULL if none exists.
 */
packet_queue *packet_pop() {
  packet_queue *tmp;
  if (head == NULL) return NULL;
  tmp = head;
  if (head == tail) { //must be only 1 packet
    tail = NULL;
  }
  head = head->next;
  tmp->next = NULL;
  //packet_inspect();
  return tmp;
}

/**
 * Frees the given packet.
 *
 * @param[in/out] p
 *    The packet.
 */
void packet_free(packet_queue *p) {
  if (p == NULL) return;
  if (p->buf != NULL) free(p->buf);
  //packet_inspect();
  //if (p->dest_addr != NULL) free(p->dest_addr);
  free(p);
}