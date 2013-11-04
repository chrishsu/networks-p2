#include "peer_whohas.h"

// Returns true iff h1 and h2 are the same hash
//
int hash_equal(char *h1, char *h2) {
  int i;
  for (i = 0; i < 20; i++) {
    if (h1[i] != h2[i])
      return 0;
  }
  return 1;
}

int has_chunk(char *hash, bt_config_t *config) {
  FILE *has_chunk_file = fopen(config->has_chunk_file, "r");
  if (has_chunk_file == NULL) {
    fprintf(stderr, "Error opening has-chunk-file\n");
    return 0;
  }

  int id;
  char buf[41];
  while (1) {
    if (fscanf(has_chunk_file, "%d %s", &id, buf) == EOF)
      break;

    uint8_t binary[20];
    hex2binary(buf, 40, binary);

    if (hash_equal(hash, (char *)binary))
      return 1;
  }
  return 0;
}

/*
 * @param[in]   buf
 *      guaranteed to have length of 1500 bytes
 */
int process_whohas(int sock, struct sockaddr_in *from, peer_header *h, bt_config_t *config) {
  #define PAD 4
  chunk_list *chunks, *next;
  short OFFSET;
  char num_chunks;
  int i, ret;

  chunks = init_chunk_list();
  next = NULL;
  //get the number of chunks
  if (h->head_len < 16) {
    printf("header is less than 16!\n");
    return -1; //something is wrong
  }
  num_chunks = h->buf[h->head_len];
  OFFSET = sizeof(packet_head) + PAD;

  //loop through
  for (i = 0; i < (int)num_chunks; i++) {
    char hash[20];
    memcpy(hash, &(h->buf[OFFSET + 20*i]), 20);

    if (has_chunk(hash, config)) {
      next = add_to_chunk_list(next, hash);
      if (i == 0)
	chunks = next;
    }
  }

  struct sockaddr_in toaddr;
  bzero(&toaddr, sizeof(toaddr));
  toaddr.sin_family = AF_INET;
  toaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  toaddr.sin_port = from->sin_port;

  ret = send_ihave(sock, &toaddr, config, chunks);
  DPRINTF(DEBUG_INIT, "Called send_ihave\n");

  //cleanup
  del_chunk_list(chunks);

  return ret;
}

// Assumes c is in a-z or 0-9
char hexVal(char c) {
  if (c < 60)
    return c - 48;
  return c - 87;
}
char charVal(char h) {
  if (h < 10)
    return h + 48;
  return h + 87;
}

int send_whohas(int sock, char *chunkfile, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Send WHOHAS\n");
    FILE *file;
    peer_header h;
    int lines = 0;
    char nl;
    int i;

    file = fopen(chunkfile, "r");
    if (file == NULL) {
      printf("No chunkfile!\n");
        return -1;
    }
    //get lines
    while ((nl = fgetc(file)) != EOF) {
        if (nl == '\n') lines++;
    }
    //printf("end loop, lines = %X\n", lines & 0xFF);
    fseek(file, 0L, SEEK_SET);

    //allocate space
    init_peer_header(&h);
    h.buf = malloc(4 + (lines * 20));
    h.buf[0] = lines & 0xFF; // Number of chunk hashes
    h.buf[1] = 0;
    h.buf[2] = 0;
    h.buf[3] = 0;

    for (i = 0; i < lines; i++) {
      int id;
      char hash_part[41];
      fscanf(file, "%d %s", &id, hash_part);

      uint8_t binary[20];
      hex2binary(hash_part, 40, binary);
      memcpy(&h.buf[4 + i * 20], binary, 20);
    }

    //header setup
    h.type = TYPE_WHOHAS;
    h.buf_len = 4 + (lines * 20);
    h.pack_len = h.buf_len + 16;


    bt_peer_t *peer = config->peers;
    while (peer != NULL) {
      if (peer->id != config->identity)
        send_udp(sock, &(peer->addr), &h, config);
      peer = peer->next;
    }
    DPRINTF(DEBUG_INIT, "Flooded peers with WHOHAS\n");

    //cleanup
    free_peer_header(&h);

    return 0;
}
