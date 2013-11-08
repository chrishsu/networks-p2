/*
 * bt_parse.c
 *
 * Initial Author: Ed Bardsley <ebardsle+441@andrew.cmu.edu>
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2 command line parsing.
 *
 */

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bt_parse.h"
#include "udp_utils.h"
#include "chunk.h"
#include "debug.h"
#include "peer_receiver.h"

static const char* const _bt_optstring = "p:c:f:m:i:d:h";

void bt_init(bt_config_t *config, int argc, char **argv) {
  bzero(config, sizeof(bt_config_t));

  strcpy(config->output_file, "output.dat");
  strcpy(config->peer_list_file, "nodes.map");
  config->argc = argc;
  config->argv = argv;

  config->cur_download = 0;
  config->cur_upload = 0;
  config->download = NULL;
  config->download_tail = NULL;
  config->upload = NULL;
  config->last_whohas = 0;
}

void bt_usage() {
  fprintf(stderr,
	  "usage:  peer [-h] [-d <debug>] -p <peerfile> -c <chunkfile> -m <maxconn>\n"
	  "            -f <master-chunk-file> -i <identity>\n");
}

void bt_help() {
  bt_usage();
  fprintf(stderr,
	  "         -h                help (this message)\n"
	  "         -p <peerfile>     The list of all peers\n"
	  "         -c <chunkfile>    The list of chunks\n"
	  "         -m <maxconn>      Max # of downloads\n"
	  "	    -f <master-chunk> The master chunk file\n"
	  "         -i <identity>     Which peer # am I?\n"
	  );
}

bt_peer_t *bt_peer_info(const bt_config_t *config, int peer_id)
{
  assert(config != NULL);

  bt_peer_t *p;
  for (p = config->peers; p != NULL; p = p->next) {
    if (p->id == peer_id) {
      return p;
    }
  }
  return NULL;
}

void bt_parse_command_line(bt_config_t *config) {
  int c, old_optind;
  bt_peer_t *p;

  int argc = config->argc;
  char **argv = config->argv;

  DPRINTF(DEBUG_INIT, "bt_parse_command_line starting\n");
  old_optind = optind;
  while ((c = getopt(argc, argv, _bt_optstring)) != -1) {
    switch(c) {
    case 'h':
      bt_help();
      exit(0);
    case 'd':
      if (set_debug(optarg) == -1) {
	fprintf(stderr, "%s:  Invalid debug argument.  Use -d list  to see debugging options.\n",
		argv[0]);
	exit(-1);
      }
      break;
    case 'p':
      strcpy(config->peer_list_file, optarg);
      break;
    case 'c':
      strcpy(config->has_chunk_file, optarg);
      break;
    case 'f':
      strcpy(config->chunk_file, optarg);
      break;
    case 'm':
      config->max_conn = atoi(optarg);
      break;
    case 'i':
      config->identity = atoi(optarg);
      break;
    default:
      bt_usage();
      exit(-1);
    }
  }

  bt_parse_peer_list(config);

  if (config->identity == 0) {
    fprintf(stderr, "bt_parse error:  Node identity must not be zero!\n");
    exit(-1);
  }
  if ((p = bt_peer_info(config, config->identity)) == NULL) {
    fprintf(stderr, "bt_parse error:  No peer information for myself (id %d)!\n",
	    config->identity);
    exit(-1);
  }
  config->myport = ntohs(p->addr.sin_port);
  assert(config->identity != 0);
  assert(strlen(config->chunk_file) != 0);
  assert(strlen(config->has_chunk_file) != 0);

  optind = old_optind;
}

void bt_parse_peer_list(bt_config_t *config) {
  FILE *f;
  bt_peer_t *node;
  char line[BT_FILENAME_LEN], hostname[BT_FILENAME_LEN];
  int nodeid, port;
  struct hostent *host;

  assert(config != NULL);

  f = fopen(config->peer_list_file, "r");
  assert(f != NULL);

  while (fgets(line, BT_FILENAME_LEN, f) != NULL) {
    if (line[0] == '#') continue;
    assert(sscanf(line, "%d %s %d", &nodeid, hostname, &port) != 0);

    node = (bt_peer_t *) malloc(sizeof(bt_peer_t));
    assert(node != NULL);

    node->id = nodeid;
    node->downloading = 0;
    node->chunk = NULL;

    host = gethostbyname(hostname);
    assert(host != NULL);
    node->addr.sin_addr.s_addr = *(in_addr_t *)host->h_addr;
    node->addr.sin_family = AF_INET;
    node->addr.sin_port = htons(port);

    node->next = config->peers;
    config->peers = node;
  }
}

void bt_dump_config(bt_config_t *config) {
  /* Print out the results of the parsing. */
  bt_peer_t *p;
  assert(config != NULL);

  printf("15-441 PROJECT 2 PEER\n\n");
  printf("chunk-file:     %s\n", config->chunk_file);
  printf("has-chunk-file: %s\n", config->has_chunk_file);
  printf("max-conn:       %d\n", config->max_conn);
  printf("peer-identity:  %d\n", config->identity);
  printf("peer-list-file: %s\n", config->peer_list_file);

  for (p = config->peers; p; p = p->next)
    printf("  peer %d: %s:%d\n", p->id, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
}

/**
 * Adds a peer to the chunk.
 */
void add_peer_list(bt_chunk_list *chunk, bt_peer_t *peer) {
  if (chunk == NULL) return;
  bt_peer_list *pl = malloc(sizeof(bt_peer_list));
  pl->peer = peer;
  pl->next = chunk->peers;
  chunk->peers = pl;
}

/**
 * Deletes a peer list from a chunk.
 */
void del_peer_list(bt_chunk_list *chunk) {
  if (chunk == NULL) return;
  bt_peer_list *pl = chunk->peers;
  while (pl != NULL) {
    bt_peer_list *tmp = pl->next;
    free(pl);
    pl = tmp;
  }
}

/**
 * Adds a packet to the chunk.
 * If received out of order, initializes empty list nodes.
 */
void add_packet_list(bt_chunk_list *chunk, int seq_num, char *data, int data_len) {
  int last_seq = 0;
  bt_packet_list *prev = NULL;
  bt_packet_list *cur = chunk->packets;

  /*
  if (cur == NULL) {
    //printf("initialize packet list\n");
    bt_packet_list *newp = malloc(sizeof(bt_packet_list));
    newp->seq_num = seq_num;
    newp->recv = 1;
    newp->data = malloc(data_len);
    memcpy(newp->data, data, data_len);
    newp->next = NULL;
    chunk->packets = newp;
    return;
  }*/

   // Check if node already exists.
  //printf("searching packet list\n");
  while (cur != NULL) {
    //printf("seqnum: %d\n", cur->seq_num);
    last_seq = cur->seq_num;
    if (last_seq == seq_num) {
      if (cur->recv) return; // already got it!
      cur->recv = 1;
      cur->data = malloc(data_len);
      cur->data_len = data_len;
      memcpy(cur->data, data, data_len);
      break;
    }
    prev = cur;
    cur = cur->next;
  }

  // Create new nodes.
  int i;
  for (i = last_seq + 1; i <= seq_num; i++) {
    bt_packet_list *newp = malloc(sizeof(bt_packet_list));
    newp->seq_num = i;
    if (i == seq_num) {
      newp->recv = 1;
      newp->data = malloc(data_len);
      memcpy(newp->data, data, data_len);
      newp->data_len = data_len;
    }
    else {
      newp->recv = 0;
      newp->data = NULL;
    }

    newp->next = NULL;

    if (chunk->packets == NULL) {
      chunk->packets = newp;
    }
    else prev->next = newp;

    prev = newp;
  }
  chunk->total_data += data_len;

  int last_gotten_upto = 0;
  cur = chunk->packets;
  while (cur != NULL) {
    if (!cur->recv)
      break;
    last_gotten_upto = cur->seq_num;
    cur = cur->next;
  }
  chunk->next_expected = last_gotten_upto + 1;
}

/**
 * Deletes a packet list from a chunk.
 */
void del_packet_list(bt_chunk_list *chunk) {
  if (chunk == NULL) return;
  bt_packet_list *pl = chunk->packets;
  while (pl != NULL) {
    bt_packet_list *tmp = pl->next;
    if (pl->data != NULL) free(pl->data);
    free(pl);
    pl = tmp;
  }
}

/**
 * Keeps a chunk in order.
 */
void add_receiver_list(bt_config_t *c, char *hash, int id) {
  assert(c != NULL);
  bt_chunk_list *chunk = malloc(sizeof(bt_chunk_list));
  memcpy(chunk->hash, hash, 20);
  chunk->id = id;
  chunk->next_expected = INIT_SEQNUM;

  if (has_chunk(hash, c)) {
    chunk->total_data = BT_CHUNK_SIZE;
    chunk->downloaded = 1;
  } else
    chunk->downloaded = 0;

  chunk->total_data = 0;
  chunk->peer = NULL;
  chunk->peers = NULL;
  chunk->packets = NULL;
  chunk->next = NULL;
  // If front of list doesn't exist:
  if (c->download == NULL) c->download = chunk;
  // If back of list does exist:
  if (c->download_tail != NULL) c->download_tail->next = chunk;
  // Add to back of list:
  c->download_tail = chunk;
  c->num_chunks++;
}

/**
 * Deletes the chunk list.
 */
void del_receiver_list(bt_config_t* c) {
  assert(c != NULL);

  bt_chunk_list *chunk = c->download;
  while (chunk != NULL) {
    bt_chunk_list *tmp = chunk->next;
    if (chunk->peers != NULL) del_peer_list(chunk);
    if (chunk->packets != NULL) del_packet_list(chunk);
    free(chunk);
    chunk = tmp;
  }
  c->download = NULL;
  c->download_tail = NULL;
}

/**
 * Adds a connection to the sender_list.
 */
void add_sender_list(bt_config_t *c, char *hash, packet **packets, int num_packets, bt_peer_t *peer) {
  #define SSTHRESH_INIT 64
  assert(c != NULL);
  bt_sender_list *sender = malloc(sizeof(bt_sender_list));
  memcpy(sender->hash, hash, 20);

  sender->packets = packets;
  sender->num_packets = num_packets;

  sender->last_acked = 0;
  sender->last_sent = 0;
  sender->window_size = 1;
  sender->recvd = 0;
  sender->ssthresh = SSTHRESH_INIT;
  sender->state = 0;
  sender->retransmit = 0;
  sender->peer = peer;
  sender->sent_time = 0;
  sender->dropped = 0;

  sender->prev = NULL;
  sender->next = c->upload;
  if (c->upload != NULL) c->upload->prev = sender;
  c->upload = sender;
}

/**
 * Finds a connection from the peer.
 */
bt_sender_list *find_sender_list(bt_config_t *c, bt_peer_t *peer) {
  bt_sender_list *sender = c->upload;
  while (sender != NULL) {
    if (sender->peer != NULL) {
      if (sender->peer->id == peer->id) return sender;
    }
    sender = sender->next;
  }
  return NULL;
}


/**
 * Deletes a connection from the sender_list.
 */
void del_sender_list(bt_config_t *c, bt_sender_list *sender) {
  assert(c != NULL);
  if (sender == NULL) return;
  bt_sender_list *before = sender->prev;
  bt_sender_list *after = sender->next;

  if (sender->packets != NULL) {
    int i;
    for (i = 0; i < sender->num_packets; i++) {
      if (sender->packets[i] != NULL) free_packet(sender->packets[i]);
    }
    free(sender->packets);
  }

  if (before != NULL) before->next = after;
  if (before == NULL) c->upload = after;
  if (after != NULL) after->prev = before;

  free(sender);
}

int master_data_file(char *file, bt_config_t *config) {
  FILE *chunk_file = fopen(config->chunk_file, "r");
  if (chunk_file == NULL) {
    fprintf(stderr, "Error opening master-chunk-file\n");
    return 0;
  }

  if (fscanf(chunk_file, "File: %s", file) < 0) {
    fclose(chunk_file);
    return 0;
  }
  fclose(chunk_file);
  return 1;
}

/**
 * Checks if this peer has a chunk for the given hash.
 *
 * @param hash Chunk to check for
 * @param config Config object
 *
 * @return The ID if this peer has the hash, -1 otherwise.
 */
int has_chunk(char *hash, bt_config_t *config) {
  FILE *has_chunk_file = fopen(config->has_chunk_file, "r");
  if (has_chunk_file == NULL) {
    fprintf(stderr, "Error opening has-chunk-file\n");
    return -1;
  }

  int id = -1;
  char buf[41];
  while (1) {
    if (fscanf(has_chunk_file, "%d %s", &id, buf) == EOF)
      break;

    uint8_t binary[20];
    hex2binary(buf, 40, binary);

    if (hash_equal(hash, (char *)binary))
      break;
  }
  fclose(has_chunk_file);
  return id;
}

/**
 * @param addr Address to find
 * @param config Config struct
 * @return Peer with address addr
 */
bt_peer_t *peer_with_addr(struct sockaddr_in *addr, bt_config_t *config) {
  bt_peer_t *current = config->peers;
  while (current != NULL) {
    if (current->addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
	current->addr.sin_port == addr->sin_port)
      return current;
    current = current->next;
    /*
    if (memcmp(addr, current->addr, sizeof(struct sockaddr_in)) == 0)
      return current;
    */
  }
  return NULL;
}

/**
 * @param config
 * Resets values for the all peers
 */
void reset_peers(bt_config_t *config) {
  bt_peer_t *cur = config->peers;
  while (cur != NULL) {
    cur->consec_timeouts = 0;
    cur->bad_time = 0;
    cur->chunk = NULL;
    cur->downloading = 0;
    // Not expecting response:
    cur->last_response = -1;
    cur = cur->next;
  }
}
