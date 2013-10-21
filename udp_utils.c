#include "upd_utils.h"

chunk_list *init_chunk_list() {
  return NULL;
}

chunk_list *add_to_chunk_list(chunk_list *list, char *hash) {
  chunk_list *new_list;
  new_list = malloc(sizeof(chunk_list));
  memcpy(new_list->hash, hash, 20);
  if (list == NULL) {
    list = new_list;
  }
  else { //list->next == NULL
    list->next = new_list;
  }
  return new_list;
}

void del_chunk_list(chunk_list *list) {
  if (list == NULL) return;
  del_chunk_list(list->next);
  free(list);
}

void init_peer_header(peer_header *h) {
  h->type = -1;
  h->pack_len = 0;
  h->buf_len = 0;
  h->seq_num = 0;
  h->ack_num = 0;
  h->buf = NULL;
}

void free_peer_header(peer_header *h) {
  if (h->buf != NULL) free(h->buf);
  h->buf = NULL;
}


int process_udp(peer_header *h) {
  return -1;
}

int send_udp(int sock, peer_header *h, void *config) {
  returns -1;
}