#include "peer_whohas.h"

// @param h1 First hash
// @param h2 Second hash
// @return Whether h1 and h2 are the same hash
int hash_equal(char *h1, char *h2) {
  int i;
  for (i = 0; i < 20; i++) {
    if (h1[i] != h2[i])
      return 0;
  }
  return 1;
}

// @param hash Chunk to check for
// @param config Config object
// @return Whether this peer has the chunk with the hash in hash
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

// @param sock Socket
// @param from From address
// @param h Header of sending peer
// @param config Config object
// @return -1 on error, or else the result of send_ihave
int process_whohas(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  #define PAD 4
  chunk_list *chunks, *next;
  char num_chunks;
  int ret;

  chunks = init_chunk_list();
  next = NULL;
  num_chunks = p->buf[0];

  //loop through
  char i;
  for (i = 0; i < num_chunks; i++) {
    char hash[20];
    memcpy(hash, &(p->buf[PAD + 20 * i]), 20);

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

// Assumes c is in a-f or 0-9
// @param c Character
// @return Value of c as a hex digit
char hexVal(char c) {
  if (c < 60)
    return c - 48;
  return c - 87;
}

// @param h Hex digit
// @return Ascii value of character representing h
char charVal(char h) {
  if (h < 10)
    return h + 48;
  return h + 87;
}

// @param sock Socket
// @param chunkfile Chunk-file filename
// @param config Config object
// @return 0 on success, -1 on error
int send_whohas(int sock, char *chunkfile, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Send WHOHAS\n");
    FILE *file;
    packet p;
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
    p.buf = malloc(PAD + (lines * 20));
    p.buf[0] = lines & 0xFF; // Number of chunk hashes
    p.buf[1] = p.buf[2] = p.buf[3] = 0;

    for (i = 0; i < lines; i++) {
      int id;
      char hash_part[41];
      fscanf(file, "%d %s", &id, hash_part);

      uint8_t binary[20];
      hex2binary(hash_part, 40, binary);
      memcpy(&p.buf[4 + i * 20], binary, 20);
    }

    //header setup
    p.header.magic_num = htons(15441);
    p.header.version = 1;
    p.header.type = TYPE_WHOHAS;
    p.header.header_len = htons(16);
    p.header.packet_len = htons(p.header.header_len + 4 + lines * 20);

    bt_peer_t *peer = config->peers;
    while (peer != NULL) {
      if (peer->id != config->identity)
        send_udp(sock, &(peer->addr), &p, config);
      peer = peer->next;
    }
    DPRINTF(DEBUG_INIT, "Flooded peers with WHOHAS\n");

    //cleanup
    free(p.buf);

    return 0;
}
