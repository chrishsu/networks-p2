#include "udp_utils.h"

long long time_millis() {
  struct timeval t;
  gettimeofday(&t, NULL);

  long long seconds = t.tv_sec;
  long long micro_frac = t.tv_usec;
  long long millis = seconds * 1000 + micro_frac / 1000;
  return millis;
}

void free_packet(packet *p) {
  if (p == NULL)
    return;
  if (p->buf != NULL)
    free(p->buf);
  free(p);
}

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


/**
 * Checks if two hashes are equal.
 * Assumes both h1 and h2 have at least 20 bytes.
 *  @param h1 First hash
 * @param h2 Second hash
 *
 * @return 1 if h1 and h2 are the same hash, 0 otherwise.
 */
int hash_equal(char *h1, char *h2) {
  int i;
  for (i = 0; i < 20; i++) {
    if (h1[i] != h2[i])
      return 0;
  }
  return 1;
}

/**
 * Initializes the packet_head.
 *
 * @param[in/out] h
 *      The packet_head.
 * @param[in] type
 * @param[in] header_len
 * @param[in] packet_len
 * @param[in] seq_num
 * @param[in] ack_num
 */
void init_packet_head(packet_head *h, char type,
                      short header_len, short packet_len,
                      int seq_num, int ack_num) {
  h->magic_num = htons(15441);
  h->version = 1;
  h->type = type;
  h->header_len = htons(header_len);
  h->packet_len = htons(packet_len);
  h->seq_num = htonl(seq_num);
  h->ack_num = htonl(ack_num);
}

/*
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
  }*/

/*
int process_udp(packet *p) {
  printf("Processing header..\n");
  short magic_num = ntohs(*(short *)(p->header.magic_num));
  char version = *(char *)(p->header.version);
  if (magic_num != 15441 ||
      version != 1) {
    DPRINTF(DEBUG_INIT, "Bad: magic_num: %d\tversion: %d\n", magic_num, version);
    return DROPPED; // Drop the packet
  }
  h->type = *(char *)(h->buf + 3);
  h->head_len = ntohs(*(short *)(h->buf + 4));
  h->pack_len = ntohs(*(short *)(h->buf + 6));
  h->buf_len = h->pack_len - h->head_len;
  h->seq_num = ntohl(*(int *)(h->buf + 8));
  h->ack_num = ntohl(*(int *)(h->buf + 12));
  return h->type;
  }*/

/*
// Assumes p has network byte order already
int send_udp(int sock, struct sockaddr_in *toaddr, packet *p, bt_config_t *config) {
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

  char *packet_buf = malloc(p->header->packet_len);
  if (!packet) {
    // Failed allocating packet string
    return -1;
  }
  memcpy(packet_buf, p, p->header->packet_len);



  memcpy(packet, &ph, sizeof(ph));
  memcpy(packet + sizeof(ph), h->buf, h->buf_len);

  struct sockaddr_in *newaddr = malloc(sizeof(struct sockaddr_in));
  *newaddr = *toaddr;
  packet_new(packet, h->pack_len, (struct sockaddr *)newaddr);

  return 0;
  }*/
