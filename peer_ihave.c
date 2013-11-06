#include "peer_ihave.h"

int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  printf("Process IHAVE\n");

  short datalen = ntohs(p->header.packet_len) - sizeof(packet_header);
  char numchunks = p->buf[0];
  datalen -= 4; // We already read the number of chunks
  char *nextchunk = p->buf + 4;
  char i;
  char hash[20];
  for (i = 0; i < numchunks; i++) {
    if (datalen < 20) {
      fprintf(stderr, "Peer packet too short!\n");
      return -1;
    }
    memcpy(hash, nextchunk, 20);
    bt_chunk_list *cur = config->download;
    while (cur != NULL) {
      if (hash_equal(hash, cur->hash)) {
	//// Add to peer list ////
	break;
      }
      cur = cur->next;
    }

    datalen -= 20;
    nextchunk += 20;
  }
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
    p.header.magic_num = htons(15441);
    p.header.version = 1;
    p.header.type = TYPE_IHAVE;
    p.header.header_len = htons(16);
    p.header.packet_len = htons(sizeof(packet_head) + 4 + 20 * num_chunks);
    p.buf = malloc(4 + 20 * num_chunks);
    if (!p.buf) {
      // Failed allocation
      return -1;
    }
    p.buf[0] = num_chunks;
    char *next_loc = p.buf + 4;
    int i;
    for (i = 0; i < num_chunks; i++) {
      memcpy(next_loc, next->hash, 20);
      next = next->next;
      next_loc += 20;
    }
    packet_new(&p, toaddr);

    total_chunks -= num_chunks;

    printf("Added IHAVE to queue with %d chunks\n", num_chunks);
  }
  return 0;
}
