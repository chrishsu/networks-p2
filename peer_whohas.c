#include "peer_whohas.h"

/*
 * @param[in]   buf
 *      guaranteed to have length of 1500 bytes
 */
int process_whohas(int sock, struct sockaddr_in *from, peer_header *h, bt_config_t *config) {
  printf("Process WHOHAS\n");
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
    OFFSET = h->pack_len + PAD;
    //printf("head_len: %d\tchunks: %X\n", h->head_len, num_chunks);

    //loop through
    for (i = 0; i < (int)num_chunks; i++) {
      char hash[20];
      memcpy(hash, &(h->buf[OFFSET + 20*i]), 20);
      next = add_to_chunk_list(next, hash);
      if (i == 0) chunks = next;
      //printf("total chunks: %d \t%d\n", i, chunk_list_len(chunks));
    }
    //printf("total chunks: %d \t%d\n", i, chunk_list_len(chunks));

    ret = send_ihave(sock, from, config, chunks);
    printf("Called send_ihave\n");

    //cleanup
    del_chunk_list(chunks);

    return ret;
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
    }

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
