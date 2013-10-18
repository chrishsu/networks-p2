#include "peer_whohas.h"

int process_whohas(int sock, struct sockaddr_in *from, void *config) {
    
}

int send_whohas(int sock, char *chunkfile, void *config) {
    FILE *file;
    peer_header h;
    int lines = 0;
    char nl;
    int i, ret;
    
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
    h.seq_num = 0;
    h.ack_num = 0;
    
    ret = send_udp(sock, &h, config);
    
    //cleanup
    free(h.buf);
    
    return ret;
}
