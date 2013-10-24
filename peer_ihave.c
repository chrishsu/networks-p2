#include "peer_ihave.h"

int process_ihave(struct sockadrr_in *from, peer_header *h, void *config) {

}

// Does nothing if chunks is empty:
int send_ihave(int sock, int peer_id, void *config, chunk_list *chunks) {
  int total_chunks = chunk_list_len(chunks);
  int max_chunks = (MAX_PACKET_SIZE - sizeof(packet_head)) / CHUNK_SIZE;

  chunk_list *next = chunk_list;
  while (total_chunks > 0) {
    char num_chunks = total_chunks;
    if (num_chunks > max_chunks)
      num_chunks = max_chunks;

    peer_header ph;
    ph.type = 1;
    ph.buf_len = 4 + CHUNK_SIZE * num_chunks;
    ph.pack_len = sizeof(packet_head) + ph.buf_len;
    // We don't care about the seq or ack nums

    char *buf = malloc(ph.buf_len);
    if (!buf) {
      // Failed allocation
      return -1;
    }
    *buf = num_chunks;
    char *next_loc = buf + 4;
    int i;
    for (i = 0; i < num_chunks; i++) {
      memcpy(next_loc, next, CHUNK_SIZE);
      next = next->next;
      next_loc += 20;
    }
    ph->buf = buf;
    int ret_val = send_udp(sock, peer_id, ph, config);
    if (ret_val < 0)
      return ret_val;
    total_chunks -= num_chunks;
  }
  return 0;
}
