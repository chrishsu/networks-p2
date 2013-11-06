#include "peer_sender.h"

/**
 * Processes a WHOHAS request.
 * 
 * @param sock Socket
 * @param from From address
 * @param p Packet recieved
 * @param config Config object
 *
 * @return -1 on error, or else the result of send_ihave.
 */
int process_whohas(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "process_whohas...\n");
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

    if (has_chunk(hash, config) >= 0) {
      next = add_to_chunk_list(next, hash);
      if (i == 0)
	chunks = next;
    }
  }

  /*
  struct sockaddr_in toaddr;
  bzero(&toaddr, sizeof(toaddr));
  toaddr.sin_family = AF_INET;
  toaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  toaddr.sin_port = from->sin_port;
  */

  ret = send_ihave(sock, from, config, chunks);
  DPRINTF(DEBUG_INIT, "Called send_ihave\n");

  //cleanup
  del_chunk_list(chunks);

  return ret;
}

/**
 * Sends an IHAVE packet to the given address.
 * Does nothing if chunks is empty.
 * 
 * @param sock Socket
 * @param toaddr To address
 * @param chunks list of chunks
 * @param config Config object
 *
 * @return -1 on error, or else 0.
 */
int send_ihave(int sock, struct sockaddr_in *to, chunk_list *chunks, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Send IHAVE\n");
  int total_chunks = chunk_list_len(chunks);
  int max_chunks = (MAX_PACKET_SIZE - sizeof(packet_head)) / CHUNK_SIZE;

  chunk_list *next = chunks;
  while (total_chunks > 0) {
    char num_chunks = total_chunks;
    if (num_chunks > max_chunks)
      num_chunks = max_chunks;

    packet p;
    init_packet_head(&p.header, TYPE_IHAVE, 16,
                     sizeof(packet_head) + 4 + 20 * num_chunks,
                     0, 0);
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
    packet_new(&p, to);

    total_chunks -= num_chunks;

    DPRINTF(DEBUG_INIT, "Added IHAVE to queue with %d chunks\n", num_chunks);
  }
  return 0;
}


/**
 * Processes a GET request then sends a DATA packet.
 * 
 * @param sock Socket
 * @param from From address
 * @param p Packet recieved
 * @param config Config object
 *
 * @return -1 on error, or else the result of send_data.
 */
int process_get(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  char hash[20];
  int ret;
  DPRINTF(DEBUG_INIT, "Process GET\n");
  
  // Only expecting 20 bytes
  if (ntohs(p->header->packet_len) - ntohs(p->header->header_len) != 20) {
    return -1;
  }
  
  memcpy(hash, p->buf, 20);
  
  ret = send_data(sock, from, hash, config);
  
  return ret;
}


/**
 * Sends DATA packet.
 * 
 * @param sock Socket
 * @param toaddr To address
 * @param hash Chunk hash of data to send
 * @param config Config object
 *
 * @return -1 on error, or else 0.
 */
int send_data(int sock, struct sockaddr_in *to, char *hash, bt_config_t *config) {
  #define MAX_DATA_SIZE 1500 - 16
  #define FILENAME_LEN 255
  int id;
  char filename[FILENAME_LEN];
  uint8_t filechunk[BT_CHUNK_SIZE];
  size_t read_bytes;
  DPRINTF(DEBUG_INIT, "Send DATA\n");
  
  // Check if we actually have the chunk
  if ((id = has_chunk(hash, config)) < 0) {
    return -1;
  }
  
  if (!master_data_file(filename)) {
    return -1;
  }
  
  FILE *master = fopen(filename, "rb"); 
  if (master == NULL) {
    fprintf(stderr, "Error opening master-data-file\n");
    return -1;
  }
  
  if (fseek(master, id*BT_CHUNK_SIZE, SEEK_SET) != 0) {
    return -1;
  }
  
  read_bytes = fread(filechunk, sizeof(uint8_t), BT_CHUNK_SIZE, master);
  if (read_bytes <= 0) return -1;
  
  // Check the hash.
  uint8_t checkhash[20];
  shahash(filechunk, BT_CHUNK_SIZE, checkhash);
  if (!hash_equal(hash, (char *) check_hash)) {
    return -1;
  }
  
  int num_packets = (int)read_bytes/MAX_DATA_SIZE;
  if (num_packets * MAX_DATA_SIZE < (int)read_bytes) num_packets++;
  
  packet *packets[] = malloc(num_packets);
  
  int i, chunk_bytes;
  // Sequence Numbers start at 1
  for (i = 1; i <= num_packets; i++) {
    int buf_len = (int)read_bytes - chunk_bytes;
    if (buf_len > MAX_DATA_SIZE) buf_len = MAX_DATA_SIZE;
    
    // Add packets
    packet *p = malloc(sizeof(packet));
    init_packet_head(&p.header, TYPE_DATA, 16, 16 + buf_len, i, 0);
    p->buf = malloc(buf_len);
    memcpy(p->buf, filechunk[chunk_bytes], buf_len);
    
    packets[i] = p;
    
    chunk_bytes += buf_len;
  }
  
  if (chunk_bytes != (int)read_bytes) {
    fprintf(stderr, "Bytes to send and bytes read not equal!\n");
    return -1;
  }
  
  // From sockaddr
  bt_peer_t *peer = peer_with_addr(to, config);
  
  add_sender_list(config, hash, packets, num_packets, peer);
  
  DPRINTF(DEBUG_INIT, "Added to %d packets for peer %d to sender list\n", num_packets, peer->id);
  
  return 0;
}

/**
 * Processes ACK with connection_list.
 * Portion of congestion control (slow start & congestion avoidance) with increases in window_size.
 * 
 * @param sock Socket
 * @param from From address
 * @param p Packet recieved
 * @param config Config object
 *
 * @return -1 on error, or else 0.
 */
int process_ack(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Process ACK\n");
  bt_peer_t *peer = peer_with_addr(from, config);
  if (peer == NULL) {
    fprintf(stderr, "Didn't find a peer!\n");
    return -1;
  }
  
  // Find from peer
  bt_sender_list *sender = find_sender_list(config, peer);
  if (sender == NULL) {
    fprintf(stderr, "Didn't find a sender!!\n");
    return -1;
  }
  
  if (sender->last_acked == p->ack_num) {
    sender->retransmit++;
  }
  if (sender->last_acked < p->ack_num) {
    sender->last_acked = p->ack_num;
  }
  if (sender->last_acked > p->ack_num) {
    // Probably not good.
  }  
  
  /* Slow Start */
  if (sender->state == 0) { //CC_START
    // Increase window size by 1
    if (sender->window_size < sender->ssthresh)
      sender->window_size++;
  }
  /* Congestion Avoidance */
  else {
    sender->recvd++;
    if (sender->recvd == sender->window_size) {
      if (sender->window_size < sender->ssthresh)
        sender->window_size++;
      sender->recvd = 0;
    }
  }
}