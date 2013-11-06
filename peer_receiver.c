#include "peer_receiver.h"

/**
 * @param addr Address to find
 * @param config Config struct
 * @return Peer with address addr
 */
bt_peer_t *peer_with_addr(struct sockaddr_in *addr, bt_config_t *config) {
  bt_peer_t *current = config->peers;
  while (current != NULL) {
    if (current->addr->sin_addr.s_addr == addr->sin_addr.saddr &&
	current->addr->sin_port == addr->sin_port)
      return current;
    /*
    if (memcmp(addr, current->addr, sizeof(struct sockaddr_in)) == 0)
      return current;
    */
  }
  return NULL;
}

// @param sock Socket
// @param chunkfile Chunk-file filename
// @param config Config object
// @return 0 on success, -1 on error
int send_whohas(int sock, bt_config_t *config) {
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
	packet_new(&p, &(peer->addr));
      peer = peer->next;
    }
    DPRINTF(DEBUG_INIT, "Flooded peers with WHOHAS\n");

    //cleanup
    free(p.buf);

    return 0;
}

int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  printf("Process IHAVE\n");
  bt_peer *peer = peer_with_addr(from, config);
  short datalen = ntohs(p->header.packet_len) - sizeof(packet_header);
  char numchunks = p->buf[0];
  datalen -= 4; // We already read the number of chunks
  char *nextchunk = p->buf + 4;
  char i;
  char hash[20];

  flag sent_get = 0;
  for (i = 0; i < numchunks; i++) {
    if (datalen < 20) {
      fprintf(stderr, "Peer packet too short!\n");
      return -1;
    }
    memcpy(hash, nextchunk, 20);
    bt_chunk_list *cur = config->download;
    while (cur != NULL) {
      if (hash_equal(hash, cur->hash)) {
	add_peer_list(cur, peer);

	if (cur->peer == NULL && !sent_get) {
	  // Doesn't do anything if we have too many connections:
	  send_get(sock, from, cur, config);
	  // However, we can still set this to true even in that case:
	  sent_get = 1;
	}
	// We found a match for that hash, don't keep looking for it!
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

/**
 * Sends a GET request for the given hash.
 */
int send_get(int sock, struct sockaddr_in *to, bt_chunk_list *chunk, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Sending GET...\n");
  if (config->cur_download >= config->max_conn)
    return -1;
  config->max_conn++;
  bt_peer_t *sending_peer = peer_with_addr(to, config);
  sending_peer->chunk = chunk;
  sending_peer->downloading = 1;
  chunk->peer = sending_peer;
  chunk->next_expected = INIT_SEQNUM;
  chunk->packets = NULL;

  packet p;
  init_packet_head(&(p.header), TYPE_GET, sizeof(packet_head), sizeof(packet_head), 0, 0);
  p.buf = NULL;
  packet_new(&p, to);
  DPRINTF(DEBUG_INIT, "Sent GET sucessfully!\n");
  return 0;
}

/**
 * Assumes you gave it a valid seq_num. If not, bad stuff happens like infinite mallocing o_O
 */
void add_packet(bt_chunk_list *chunk, packet *p) {
  bt_packet_list *list = chunk->packets;
  int seq_num = ntohl(p->header.seq_num);
  size_t data_len = ntohs(p->header.packet_len) - sizeof(packet_header);

  chunk->total_data += data_len;
  if (list == NULL) {
    list = malloc(sizeof(bt_packet_list));
    list->seq_num = INIT_SEQNUM;
    list->recv = 0; // Haven't received it
    list->next = NULL;
  }
  while (list->next != NULL && list->seq_num < seq_num)
    list = list->next;
  while (list->seq_num < seq_num) { // meaning list->next is NULL
    // Add more elements to the end of the list:
    bt_packet_list *extra = malloc(sizeof(bt_packet_list));
    extra->seq_num = list->seq_num + 1;
    extra->recv = 0;
    extra->next = NULL;
    list->next = extra;
    list = list->next;
  }
  // Yay! list->seqnum == seq_num!!
  list->recv = 1; // We got it
  list->data = malloc(data_len);
  memcpy(list->data, p->buf, data_len);
  list->data_len = data_len;

  if (seq_num == chunk->next_expected) {
    int largest_gotten_upto = seq_num;
    while (list != NULL) {
      if (!list->recv)
	break;
      largest_gotten_upto = list->seq_num;
      list = list->next;
    }
    chunk->next_expected = largest_gotten_upto;
  }

  if (total_data == BT_CHUNK_SIZE)
    finish_chunk(sock, chunk, config);
}

void finish_chunk(int sock, bt_chunk_list *chunk, bt_config_t *config) {
  FILE *hcf = fopen(config->has_chunk_file, "a");
  if (!hcf) {
    fprintf("Error opening has-chunk-file to append!\n");
    return;
  }
  char hash_text[41];
  binary2hex(chunk->hash, 20, hash_text);
  fprintf(hcf, "%d %s\n", chunk->id, hash_text);
  fclose(hcf);

  config->num_downloaded++;

  bt_peer_t *peer = chunk->peer;
  chunk->peer->downloading = 0;
  chunk->peer->chunk = NULL;
  chunk->peer = NULL;

  if (config->num_downloaded == num_chunks) {
    // DONE!!! :)
    finish_get(config);
  } else {
    // More to do...
    bt_chunk_list *chunk = peer->chunk;
    while (chunk != NULL) {
      // If it's not being downloaded...
      if (chunk->peer == NULL) {
	// Does the necessary setup:
	send_get(sock, peer->addr, chunk, config);
	break;
      }
    }
  }
}

bt_chunk_list *chunk_with_id(int id, bt_config_t *config) {
  bt_chunk_list *cur = config->download;
  while (cur != NULL) {
    if (cur->id == id)
      return ur;
    cur = cur->next;
  }
  return NULL;
}

void finish_get(bt_config_t *config) {
  FILE *outfile = fopen(config->output_file, "wb");
  if (!outfile) {
    fprintf(stderr, "Error opening output file!\n");
    return;
  }
  int next_chunk;
  for (next_chunk = 0; next_chunk < config->num_chunks; next_chunk++) {
    bt_chunk_list *next = chunk_with_id(next_chunk, config);
    write_to_file(next->packets, outfile);
  }
  fclose(outfile);

  del_receiver_list(config);
}

/**
 * Adds the data to the master chunk_list, sends an ACK as well.
 */
int process_data(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "process_data...\n");
  int seq_num = ntohl(p->header.seq_num);
  if (seq_num > MAX_SEQNUM || seq_num < INIT_SEQNUM) {
    fprintf(stderr, "Bad SEQNUM!\n");
    return -1;
  }

  bt_peer_t *peer = peer_with_addr(from, config);
  add_packet(peer->chunk, p);
  send_ack(sock, from, peer->chunk->next_expected);
}

/**
 * Sends an ACK with nextExpected chunk index in the master chunk_list.
 */
int send_ack(int sock, struct sockaddr_in *to, int ack_num) {
  packet p;
  init_packet_head(&(p.header), TYPE_ACK, sizeof(packet_head), sizeof(packet_head), 0, ack_num);
}

/**
 * Writes the data in the packets in the given list to the given file pointer
 */
void write_to_file(bt_packet_list *packets, FILE *outfile) {
  while (packets != NULL) {
    fwrite(packets->data, sizeof(char), packets->data_len, outfile);
    packets = packets->next;
  }
}
