#include "peer_whohas.h"

/*
 * @param[in]   buf
 *      guaranteed to have length of 1500 bytes
 */
int process_whohas(int sock, struct sockaddr_in *from, peer_header *h, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Process WHOHAS\n");

    #define UDP 8 //UDP header
    #define PAD 4
    chunk_list *chunks, *next;
    short num_chunks, OFFSET;
    int i, ret;

    chunks = init_chunk_list();
    next = chunks;
    //get the number of chunks
    num_chunks = ntohs(h->buf[UDP + h->pack_len]); //correct usage of ntohs?
    OFFSET = UDP + h->pack_len + PAD;

    //loop through
    for (i = 0; i < num_chunks; i++) {
        char hash[20];
        memcpy(hash, &(h->buf[OFFSET + 20*i]), 20);
        next = add_to_chunk_list(next, hash);
        if (i == 0) chunks = next;
    }

    ret = send_ihave(sock, from, config, chunks);
    DPRINTF(DEBUG_INIT, "Called send_ihave\n");

    //cleanup
    del_chunk_list(chunks);

    return ret;
}

int send_whohas(int sock, char *chunkfile, bt_config_t *config) {
    FILE *file;
    peer_header h;
    int lines = 0;
    char nl;
    int i;

    file = fopen(chunkfile, "r");
    if (file == NULL) {
        return -1;
    }
    //get lines
    while ((nl = fgetc(file)) != '\0') {
        if (nl == '\n') lines++;
    }
    fseek(file, 0L, SEEK_SET);

    //allocate space
    init_peer_header(&h);
    h.buf = malloc(4 + (lines * 20));
    h.buf[0] = lines & 0xFF; // Number of chunk hashes

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

    //header setup
    h.type = TYPE_WHOHAS;
    h.buf_len = 4 + (lines * 20);



    bt_peer_t *peer = config->peers;
    while (peer != NULL) {
      send_udp(sock, &(peer->addr), &h, config);
      peer = peer->next;
    }
    DPRINTF(DEBUG_INIT, "Flooded peers with WHOHAS\n");

      /*
    ret = send_udp(sock, &h, config);
      */

    //cleanup
    free_peer_header(&h);

    return 0;
}
