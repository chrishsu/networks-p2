#include "peer_whohas.h"

/*
 * @param[in]   buf
 *      guaranteed to have length of 1500 bytes
 */
int process_whohas(int sock, struct sockaddr_in *from, peer_header *h, bt_config_t *config) {
  printf("Process WHOHAS\n");
  printf("h->buf: \n");
  int k;
  for (k = 0; k < h->buf_len; k++) {
    uint8_t c = (h->buf)[k];
    char *hex = (char *)malloc(2);
    binary2hex(&c, 1, hex);
    printf("%s ", hex);
    free(hex);
  }
  printf("\n\n");

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
    num_chunks = h->buf[h->head_len]; //correct usage of ntohs?
    OFFSET = sizeof(packet_head) + PAD;
    //printf("head_len: %d\tchunks: %X\n", h->head_len, num_chunks);

    //loop through
    for (i = 0; i < (int)num_chunks; i++) {
      char hash[20];
      memcpy(hash, &(h->buf[OFFSET + 20*i]), 20);

      int k;
      for (k = 0; k < 20; k++) {
	uint8_t c = (hash)[k];
	char *hex = (char *)malloc(2);
	binary2hex(&c, 1, hex);
	printf("%s ", hex);
	free(hex);
      }

      char testHash[21];
      memcpy(testHash, hash, 20);
      testHash[20] = 0;
      printf("Copied hash: '%s'\n", testHash);

      next = add_to_chunk_list(next, hash);
      if (i == 0) chunks = next;
      //printf("total chunks: %d \t%d\n", i, chunk_list_len(chunks));
    }
    //printf("total chunks: %d \t%d\n", i, chunk_list_len(chunks));

    struct sockaddr_in toaddr;
    bzero(&toaddr, sizeof(toaddr));
    toaddr.sin_family = AF_INET;
    toaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    toaddr.sin_port = from->sin_port;

    ret = send_ihave(sock, &toaddr, config, chunks);
    printf("Called send_ihave\n");

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
  printf("Send WHOHAS\n");
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
      /*      char *line = malloc(40);
      size_t numBytes = 40;
      getline(&line, &numBytes, file);
      printf("Got '%s'\n", line);*/
      int id;
      char hash_part[41];
      fscanf(file, "%d %s", &id, hash_part);

      uint8_t binary[20];
      hex2binary(hash_part, 40, binary);
      memcpy(&h.buf[4 + i * 20], binary, 20);
    }

    printf("h: ");
    for (i = 0; i < 4 + lines * 20; i++) {
      uint8_t c = h.buf[i];
      char *hex = (char *)malloc(2);
      binary2hex(&c, 1, hex);
      printf("%s ", hex);
      free(hex);
    }
    printf("\n\n");

    /*
    //get hashes
    for (i = 0; i < lines; i++) {
        short place, idx;
        place = 0; idx = 0;
        while ((nl = fgetc(file)) != '\n') {
            if (place) {
                h.buf[4+((i+1)*idx)] = nl;
            }
            if (nl == ' ') place = 1;
        }
	}*/

    //printf("BUFFER: %X\n\n", h.buf[0]);

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
    printf("Flooded peers with WHOHAS\n");

      /*
    ret = send_udp(sock, &h, config);
      */

    //cleanup
    free_peer_header(&h);

    return 0;
}
