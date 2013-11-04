#include "peer_ihave.h"

int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  printf("Process IHAVE\n");
  return 0;
}

// Does nothing if chunks is empty:
int send_ihave(int sock, struct sockaddr_in *toaddr, bt_config_t *config, chunk_list *chunks) {
  printf("Send IHAVE\n");
  int total_chunks = chunk_list_len(chunks);
  int max_chunks = (MAX_PACKET_SIZE - sizeof(packet_head)) / CHUNK_SIZE;

  chunk_list *next = chunks;
  while (total_chunks > 0) {
    char num_chunks = total_chunks;
    if (num_chunks > max_chunks)
      num_chunks = max_chunks;

    packet p;
    peer_header ph;
    p.header.type = 1;
    p.header.buf_len = 4 + CHUNK_SIZE * num_chunks;
    p.header.packet_len = sizeof(packet_head) + ph.buf_len;
    // We don't care about the seq or ack nums

    //printf("total chunks: %d\n", total_chunks);
    char *buf = malloc(ph.buf_len);
    if (!buf) {
      // Failed allocation
      return -1;
    }
    buf[0] = num_chunks;
    char *next_loc = buf + 4;
    int i;
    for (i = 0; i < num_chunks; i++) {
      memcpy(next_loc, next, CHUNK_SIZE);
      next = next->next;
      next_loc += 20;
    }
    packet_new(&p, toaddr);

    total_chunks -= num_chunks;

    printf("Added IHAVE to queue with %d chunks\n", num_chunks);
    free(buf); buf = NULL;
  }
  return 0;
}
