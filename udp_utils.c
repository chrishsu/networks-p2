#include "udp_utils.h"

chunk_list *init_chunk_list() {
  return NULL;
}

chunk_list *add_to_chunk_list(chunk_list *list, char *hash) {
  chunk_list *new_list;
  new_list = malloc(sizeof(chunk_list));
  memcpy(new_list->hash, hash, 20);
  new_list->next = NULL;
  if (list == NULL) {
    //printf("list == NULL\n");
    list = new_list;
  }
  else { //list->next == NULL
    //printf("list != NULL\n");
    list->next = new_list;
  }
  return new_list;
}

int chunk_list_len(chunk_list *list) {
  if (list == NULL)
    return 0;
  return 1 + chunk_list_len(list->next);
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
  printf("Processing header..\n");
  short magic_num = ntohs(*(short *)(h->buf));
  char version = *(char *)(h->buf + 2);
  if (magic_num != 15441 ||
      version != 1) {
    //printf("magic_num: %d\tversion: %d\n", magic_num, version);
    return DROPPED; // Drop the packet
  }
  h->type = *(char *)(h->buf + 3);
  h->head_len = ntohs(*(short *)(h->buf + 4));
  h->pack_len = ntohs(*(short *)(h->buf + 6));
  h->buf_len = h->pack_len - h->head_len;
  h->seq_num = ntohl(*(int *)(h->buf + 8));
  h->ack_num = ntohl(*(int *)(h->buf + 12));
  return h->type;
}

int send_udp(int sock, struct sockaddr_in *toaddr, peer_header *h, bt_config_t *config) {
  printf("Sending udp...\n");
  packet_head ph;
  ph.magic_num = htons(15441);
  ph.version = 1; // No byte conversion required
  ph.type = h->type; // Same as above
  ph.header_len = htons(h->pack_len - h->buf_len);
  ph.packet_len = htons(h->pack_len);
  ph.seq_num = htonl(h->seq_num);
  ph.ack_num = htonl(h->ack_num);

  //printf("Packet len = %d\n", (int)(h->pack_len));

  char *packet = malloc(h->pack_len);
  if (!packet) {
    // Failed allocating packet string
    return -1;
  }
  memcpy(packet, &ph, sizeof(ph));
  strncpy(packet + sizeof(ph), h->buf, h->buf_len);

  packet_new(packet, h->pack_len, (struct sockaddr *)toaddr);
  if (packet_empty())
    printf("Empty!!\n");

  return 0;
}
