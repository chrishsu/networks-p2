#include "peer_receiver.h"
#include "sha.h"

void timeout_check(int sock, bt_config_t *config) {
  long long cur_time = time_millis();
  bt_peer_t *cur = config->peers;
  while (cur != NULL) {
    if (cur->last_response != -1 &&
	cur_time - cur->last_response > TIMEOUT_MILLIS) {
      cur->bad = 1;
      cur->downloading = 0;
      cur->chunk->peer = NULL;
      cur->chunk = NULL;
    }
    cur = cur->next;
  }
}

void try_to_get(int sock, bt_chunk_list *chunk, bt_config_t *config) {
  if (chunk->peer != NULL) {
    fprintf(stderr, "Uh oh! Trying to get a chunk we're currently downloading!!\n");
  }
  bt_peer_list *cur = chunk->peers;
  while (cur != NULL) {
    if (!cur->peer->downloading && !cur->peer->bad) {
      send_get(sock, &(cur->peer->addr), chunk, config);
      return;
    }
  }
  // Reflood the network looking for the chunk:
  send_whohas(sock, config);
}

/**
 * @param sock Socket
 * @param chunkfile Chunk-file filename
 * @param config Config object
 * @return 0 on success, -1 on error
 */
int send_whohas(int sock, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Send WHOHAS\n");
    packet p;

    //allocate space
    p.buf = malloc(4 + 20 * config->num_chunks);
    p.buf[0] = config->num_chunks & 0xFF; // Number of chunk hashes
    p.buf[1] = p.buf[2] = p.buf[3] = 0;
    bt_chunk_list *cur = config->download;

    int i = 0;
    while (cur != NULL) {
      memcpy(&p.buf[4 + 20 * i], cur->hash, 20);
      i++;
      cur = cur->next;
    }

    init_packet_head(&(p.header), TYPE_WHOHAS, 16,
                     16 + 4 + config->num_chunks * 20, 0, 0);

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

// Can't assume that downloading just started! This might be
// after we've been downloading for a while and a peer when down.
int process_ihave(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  printf("Process IHAVE\n");
  bt_peer_t *peer = peer_with_addr(from, config);
  peer->bad = 0;

  short datalen = ntohs(p->header.packet_len) - sizeof(packet_head);
  char numchunks = p->buf[0];
  printf("numchunks = %d\n", numchunks);
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

    uint8_t uhash[20];
    memcpy(uhash, hash, 20);
    char hashtext[41];
    binary2hex(uhash, 20, hashtext);
    DPRINTF(DEBUG_INIT, "Looking for hash '%s'\n", hashtext);
    while (cur != NULL) {
      if (hash_equal(hash, cur->hash)) {
	DPRINTF(DEBUG_INIT, "Adding hash...\n");
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
  init_packet_head(&(p.header), TYPE_GET, sizeof(packet_head), sizeof(packet_head) + 20, 0, 0);
  p.buf = malloc(20);
  memcpy(p.buf, chunk->hash, 20);
  packet_new(&p, to);
  DPRINTF(DEBUG_INIT, "Sent GET sucessfully!\n");
  free(p.buf);
  return 0;
}

/**
 * Assumes you gave it a valid seq_num. If not, bad stuff happens like infinite mallocing o_O
 */
void add_packet(int sock, bt_chunk_list *chunk, packet *p, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Add packet #%d\n", ntohl(p->header.seq_num));
  int seq_num = ntohl(p->header.seq_num);
  size_t data_len = ntohs(p->header.packet_len) - sizeof(packet_head);

  // Well that's a lot shorter now! Thanks Chris! :)
  add_packet_list(chunk, seq_num, p->buf, data_len);

  printf("Total received: %d, total expected: %d\n", (int)chunk->total_data, (int)BT_CHUNK_SIZE);
  printf("Next expected: %d\n", chunk->next_expected);

  // Put this into process_data because
  // we want to send the ack before doing this:
  if (chunk->total_data == BT_CHUNK_SIZE) {
    finish_chunk(sock, chunk, config);
  }
}

int master_chunk(char *hash, bt_config_t *config) {
  FILE *chunk_file = fopen(config->chunk_file, "r");
  if (chunk_file == NULL) {
    fprintf(stderr, "Error opening master-chunk-file\n");
    return -1;
  }

  int id = -1;
  char buf[41];
  while (1) {
    int ret = fscanf(chunk_file, "%d %s", &id, buf);
    if (ret == EOF) break;
    if (ret == 0) { //read to new line
      while (fgetc(chunk_file) != '\n') { }
      continue;
    }

    uint8_t binary[20];
    hex2binary(buf, 40, binary);

    if (hash_equal(hash, (char *)binary))
      break;
  }
  fclose(chunk_file);
  return id;
}

void finish_chunk(int sock, bt_chunk_list *chunk, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Finished chunk!!\n");

  chunk->peer->downloading = 0;
  chunk->peer->chunk = NULL;
  chunk->peer = NULL;
  config->cur_download--;

  char buffer[BT_CHUNK_SIZE];
  write_to_buffer(chunk->packets, buffer);
  uint8_t computed_hash[20];
  /*SHA1Context context;
  SHA1Init(&context);
  SHA1Update(&context, buffer, BT_CHUNK_SIZE);
  SHA1Final(&context, computed_hash);*/
  shahash((uint8_t *)buffer, BT_CHUNK_SIZE, computed_hash);
  if (!hash_equal(chunk->hash, (char *)computed_hash)) {
    DPRINTF(DEBUG_INIT, "Hash doesn't match!\n");
    try_to_get(sock, chunk, config);
    return;
  }

  FILE *hcf = fopen(config->has_chunk_file, "a");
  if (!hcf) {
    fprintf(stderr, "Error opening has-chunk-file to append!\n");
    return;
  }

  // Record that we have the chunk:
  uint8_t hash[20];
  char hash_text[41];
  memcpy(hash, chunk->hash, 20);
  binary2hex(hash, 20, hash_text);
  printf("getting chunk_id...\n");
  int chunk_id = master_chunk(chunk->hash, config);
  fprintf(hcf, "%d %s\n", chunk_id, hash_text);
  fclose(hcf);
  printf("Recorded getting chunk with id %d\n", chunk_id);

  // We have another chunk now:
  config->num_downloaded++;
  chunk->downloaded = 1;

  // Reset configuration vars:
  //bt_peer_t *peer = chunk->peer;
  /*
  chunk->peer->downloading = 0;
  chunk->peer->chunk = NULL;
  chunk->peer = NULL;
  */

  if (config->num_downloaded == config->num_chunks) {
    // DONE!!! :)
    finish_get(config);
  } else {
    /*
    // More to do...
    bt_chunk_list *cur = config->download;
    while (cur != NULL) {
      // If it's not being downloaded...
      if (!cur->downloaded && cur->peer == NULL) {
      	// Does the necessary setup:
      	send_get(sock, &(peer->addr), cur, config);
      	break;
      }
      cur = cur->next;
    }
    */
  }
}

void get_all_the_thingz(int sock, bt_config_t *config) {
  bt_chunk_list *chunk = config->download;
  // Technically don't need to check max_conn, since send_get
  // does nothing if you're full, but it's faster this way:
  while (chunk != NULL && config->cur_download < config->max_conn) {
    if (!chunk->downloaded && chunk->peer == NULL) {
      bt_peer_list *peer_list = chunk->peers;
      while (peer_list != NULL) {
	if (peer_list->peer->downloading || peer_list->peer->bad)
	  continue;
	send_get(sock, &(peer_list->peer->addr), chunk, config);
	break;
	peer_list = peer_list->next;
      }
    }
    chunk = chunk->next;
  }
}

bt_chunk_list *chunk_with_id(int id, bt_config_t *config) {
  bt_chunk_list *cur = config->download;
  while (cur != NULL) {
    if (cur->id == id)
      return cur;
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
  if (peer == NULL) {
    fprintf(stderr, "Peer not found :(\n");
    return -1;
  }
  bt_chunk_list *chunk = peer->chunk;
  add_packet(sock, chunk, p, config);
  printf("Next Expected: %d\n", chunk->next_expected);
  send_ack(sock, from, chunk->next_expected - 1);

  return 0;
}

/**
 * Sends an ACK with nextExpected chunk index in the master chunk_list.
 */
int send_ack(int sock, struct sockaddr_in *to, int ack_num) {
  packet p;
  init_packet_head(&(p.header), TYPE_ACK, sizeof(packet_head), sizeof(packet_head), 0, ack_num);
  packet_new(&p, to);
  return 0;
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

/**
 * Writes the data in the packets in the given list to the given buffer
 */
void write_to_buffer(bt_packet_list *packets, char *buffer) {
  while (packets != NULL) {
    //DPRINTF(DEBUG_INIT, "%d: %d\t", packets->seq_num, (int)packets->data_len);
    memcpy(buffer, packets->data, packets->data_len);
    buffer += packets->data_len;
    packets = packets->next;
  }
  DPRINTF(DEBUG_INIT, "\n");
}
