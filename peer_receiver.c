#include "peer_receiver.h"
#include "sha.h"

void timeout_check(int sock, bt_config_t *config) {
  if (config->download == NULL)
    return;

  long long cur_time = time_millis();
  //printf("Time = %lld\n", cur_time);
  bt_peer_t *cur = config->peers;
  while (cur != NULL) {
    //printf("last_response: %lld\n", cur->last_response);
    if (cur->last_response != -1 &&
	cur_time - cur->last_response > TIMEOUT_MILLIS) {
      cur->consec_timeouts++;

      if (cur->consec_timeouts >= MAX_TIMEOUTS) {
	DPRINTF(DEBUG_INIT, "Timed out too many times! Calling it bad now!\n");
	cur->bad_time = time_millis();
	cur->downloading = 0;

	// Reset the chunk:
	cur->chunk->peer = NULL;
	cur->chunk->next_expected = INIT_SEQNUM;
	cur->chunk->total_data = 0;
	cur->chunk->downloaded = 0; // hopefully not necessary
        del_packet_list(cur->chunk);

	cur->last_response = -1;
	cur->chunk = NULL;
      }
    }
    cur = cur->next;
  }
}

int is_bad(bt_peer_t *peer) {
  return peer->bad_time != 0 &&
         (time_millis() - peer->bad_time < BAD_RESET_MILLIS);
}

void try_to_get(int sock, bt_chunk_list *chunk, bt_config_t *config) {
  if (chunk->peer != NULL) {
    fprintf(stderr, "Uh oh! Trying to get a chunk we're currently downloading!!\n");
  }
  bt_peer_list *cur = chunk->peers;
  while (cur != NULL) {
    if (!cur->peer->downloading && !is_bad(cur->peer)) {
      printf("try_to_get!\n");
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
  if (time_millis() - config->last_whohas < WHOHAS_MILLIS) {
    //DPRINTF(DEBUG_INIT, "Canceling WHOHAS\n");
    return 0;
  }
  config->last_whohas = time_millis();

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
  DPRINTF(DEBUG_INIT, "Process IHAVE\n");
  bt_peer_t *peer = peer_with_addr(from, config);
  peer->consec_timeouts = 0;
  peer->bad_time = 0;

  short datalen = ntohs(p->header.packet_len) - sizeof(packet_head);
  char numchunks = p->buf[0];
  DPRINTF(DEBUG_INIT, "numchunks = %d\n", numchunks);
  datalen -= 4; // We already read the number of chunks
  char *nextchunk = p->buf + 4;
  char i;
  char hash[20];

  //flag sent_get = 0;
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

	/*
	if (cur->peer == NULL && !sent_get) {
	  // Doesn't do anything if we have too many connections:
	  send_get(sock, from, cur, config);
	  // However, we can still set this to true even in that case:
	  sent_get = 1;
	  }*/
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
  DPRINTF(DEBUG_INIT, "Sending GET for chunk %d...\n", chunk->id);
  if (config->cur_download >= config->max_conn)
    return -1;
  config->max_conn++;
  bt_peer_t *sending_peer = peer_with_addr(to, config);
  sending_peer->chunk = chunk;
  sending_peer->downloading = 1;
  sending_peer->last_response = time_millis();
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
int add_packet(int sock, bt_chunk_list *chunk, packet *p, bt_config_t *config) {
  //DPRINTF(DEBUG_INIT, "Got packet #%d from peer %d\n", ntohl(p->header.seq_num));
  int seq_num = ntohl(p->header.seq_num);
  size_t data_len = ntohs(p->header.packet_len) - sizeof(packet_head);

  // Well that's a lot shorter now! Thanks Chris! :)
  add_packet_list(chunk, seq_num, p->buf, data_len);

  DPRINTF(DEBUG_INIT, "Total received: %d, total expected: %d\n", (int)chunk->total_data, (int)BT_CHUNK_SIZE);
  DPRINTF(DEBUG_INIT, "Next expected: %d\n", chunk->next_expected);
  // We may end up freeing everything, so save this:
  int next_expected = chunk->next_expected;

  // Put this into process_data because
  // we want to send the ack before doing this:
  if (chunk->total_data >= BT_CHUNK_SIZE) {
    if (chunk->total_data > BT_CHUNK_SIZE) {
      fprintf(stderr, "Shouldn't get here!!!\n");
    }
    finish_chunk(sock, chunk, config);
  }
  return next_expected;
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
  chunk->peer->last_response = -1;
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
    // We automatically try everything, so unnecessary:
    //try_to_get(sock, chunk, config);
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
  int chunk_id = master_chunk(chunk->hash, config);
  fprintf(hcf, "%d %s\n", chunk_id, hash_text);
  fclose(hcf);

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

/**
 * @param sock Socket to send out of
 * @param config Config
 * Examines every chunk and tries to see if it's possible
 * to GET it.
 */
void get_all_the_thingz(int sock, bt_config_t *config) {
  bt_chunk_list *chunk = config->download;
  // Technically don't need to check max_conn, since send_get
  // does nothing if you're full, but it's faster this way:
  while (chunk != NULL && config->cur_download < config->max_conn) {
    char texthash[41];
    uint8_t copyhash[20];
    memcpy(copyhash, chunk->hash, 20);
    binary2hex(copyhash, 20, texthash);

    printf("chunk %d: \n", chunk->id);
    printf("\thash: %s\n", texthash);
    printf("\tdownloaded: %d\n", chunk->downloaded);
    printf("\ttotal_data: %d\n", (int)chunk->total_data);
    printf("\t(has a) peer: %d\n", (chunk->peer != NULL));

    int all_bad = 1;
    if (!chunk->downloaded && chunk->peer == NULL) {
      bt_peer_list *peer_list = chunk->peers;
      while (peer_list != NULL) {
	if (!is_bad(peer_list->peer))
	  all_bad = 0;
	if (!peer_list->peer->downloading && !is_bad(peer_list->peer)) {
	  //printf("Current time: %lld\n", time_millis());
	  //printf("Bad time: %lld\n", peer_list->peer->bad_time);
	  printf("GETting from get_all...\n");
	  send_get(sock, &(peer_list->peer->addr), chunk, config);
	  break;
	}
	peer_list = peer_list->next;
      }
      if (all_bad) {
	send_whohas(sock, config);
      }
    }
    chunk = chunk->next;
  }
}

/**
 * @param id Chunk ID
 * @param config Config
 * @return The chunk with the given ID.
 */
bt_chunk_list *chunk_with_id(int id, bt_config_t *config) {
  bt_chunk_list *cur = config->download;
  while (cur != NULL) {
    if (cur->id == id)
      return cur;
    cur = cur->next;
  }
  return NULL;
}

/**
 * @param config Config
 * Finishes up a user GET request:
 */
void finish_get(bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "finish_get!!!\n");
  FILE *outfile = fopen(config->output_file, "wb");
  if (!outfile) {
    fprintf(stderr, "Error opening output file!\n");
    return;
  } else {
    DPRINTF(DEBUG_INIT, "Opened '%s'\n", config->output_file);
  }


  int next_chunk;
  for (next_chunk = 0; next_chunk < config->num_chunks; next_chunk++) {
    bt_chunk_list *next = chunk_with_id(next_chunk, config);

    if (0 && has_chunk(next->hash, config) != -1) {
      printf("I own chunk %d!\n", next->id);

      int id = next->id;
      char filename[255];
      uint8_t filechunk[BT_CHUNK_SIZE];
      size_t read_bytes;

      if (!master_data_file(filename, config)) {
	fprintf(stderr, "Error getting master data file!\n");
	return;
      }

      FILE *master = fopen(filename, "rb");
      if (master == NULL) {
	fprintf(stderr, "Error opening master-data-file '%s'\n", filename);
	return;
      }

      if (fseek(master, id*BT_CHUNK_SIZE, SEEK_SET) != 0) {
	fprintf(stderr, "Error in fseek!\n");
	return;
      }

      read_bytes = fread(filechunk, sizeof(uint8_t), BT_CHUNK_SIZE, master);
      if (read_bytes <= 0) {
	fprintf(stderr, "Read nonpositive number of bytes!\n");
	return;
      }
      DPRINTF(DEBUG_INIT, "Read %d bytes from hash #%d\n", (int)read_bytes, id);

      // Check the hash.
      uint8_t checkhash[20];
      shahash(filechunk, BT_CHUNK_SIZE, checkhash);
      if (!hash_equal(next->hash, (char *) checkhash)) {
	char dahash1[41];
	char dahash2[41];
	binary2hex((uint8_t *)next->hash, 20, dahash1);
	binary2hex(checkhash, 20, dahash2);
	fprintf(stderr, "%s != %s\n", dahash1, dahash2);
	fprintf(stderr, "Hashes not equal!\n");
	return;
      }

      fwrite(filechunk, sizeof(char), BT_CHUNK_SIZE, outfile);
    } else {
      write_to_file(next->packets, outfile);
    }
    next = next->next;
  }
  fclose(outfile);

  printf("GOT %s\n", config->get_chunk_file);
  del_receiver_list(config);
}

/**
 * Adds the data to the master chunk_list, sends an ACK as well.
 */
int process_data(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  bt_peer_t *peer = peer_with_addr(from, config);
  if (peer == NULL || peer->chunk == NULL) {
    DPRINTF(DEBUG_INIT, "Throwing out DATA from invalid peer or one we're not getting from\n");
    return 0;
  }

  peer->consec_timeouts = 0;
  peer->bad_time = 0;
  peer->last_response = time_millis();

  int seq_num = ntohl(p->header.seq_num);
  DPRINTF(DEBUG_INIT, "DATA %d from peer %d\n", seq_num, peer->id);
  if (seq_num > MAX_SEQNUM || seq_num < INIT_SEQNUM) {
    fprintf(stderr, "Bad SEQNUM!\n");
    return -1;
  }

  bt_chunk_list *chunk = peer->chunk;
  int next_expected = add_packet(sock, chunk, p, config);
  // Careful, add_packet might have freed this stuff:
  //printf("Got %d, next expected if any: %d\n", ntohl(p->header.seq_num), chunk->next_expected);
  printf("\tSending ACK %d to peer %d\n", next_expected - 1, peer->id);
  send_ack(sock, from, next_expected - 1);

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
    //DPRINTF(DEBUG_INIT, "Writing %d bytes to file...\n", (int)packets->data_len);
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
