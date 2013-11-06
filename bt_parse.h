/*
 * bt_parse.h
 *
 * Initial Author: Ed Bardsley <ebardsle+441@andrew.cmu.edu>
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2 command line and config file parsing
 * stubs.
 *
 */

#ifndef _BT_PARSE_H_
#define _BT_PARSE_H_

#include "udp_utils.h"

#define BT_FILENAME_LEN 255
#define BT_MAX_PEERS 1024

typedef char flag;

struct bt_chunk_list;

typedef struct bt_peer_s {
  short id;
  // True iff we're downloading from this peer:
  flag downloading;
  flag bad;
  long long last_response;
  struct bt_chunk_list *chunk;
  struct sockaddr_in addr;
  struct bt_peer_s *next;
} bt_peer_t;

typedef struct bt_peer_list {
  bt_peer_t *peer;
  struct bt_peer_list *next;
} bt_peer_list;

typedef struct bt_packet_list {
  int seq_num;
  flag recv;
  char *data;
  size_t data_len;
  struct bt_packet_list *next;
} bt_packet_list;

typedef struct bt_chunk_list {
  int id;
  char hash[20];
  int next_expected;
  size_t total_data;
  bt_peer_t *peer;
  bt_peer_list *peers;
  bt_packet_list *packets;
  struct bt_chunk_list *next;
} bt_chunk_list;

typedef struct bt_sender_list {
  char hash[20];
  packet **packets;
  int num_packets;
  int last_acked;
  int last_sent;
  int window_size;
  int recvd;
  int ssthresh;
  flag state; // start or avoid
  char retransmit;
  bt_peer_t *peer;
  struct bt_sender_list *prev;
  struct bt_sender_list *next;
} bt_sender_list;

struct bt_config_s {
  char  chunk_file[BT_FILENAME_LEN];
  char  has_chunk_file[BT_FILENAME_LEN];
  char  output_file[BT_FILENAME_LEN];
  char  peer_list_file[BT_FILENAME_LEN];
  int   max_conn;
  short identity;
  unsigned short myport;

  int argc;
  char **argv;

  bt_peer_t *peers;

  int cur_download;
  int cur_upload;
  int num_chunks;
  int num_downloaded;
  bt_chunk_list *download;
  bt_chunk_list *download_tail; // DO NOT ACCESS
  bt_sender_list *upload;
};
typedef struct bt_config_s bt_config_t;


void bt_init(bt_config_t *c, int argc, char **argv);
void bt_parse_command_line(bt_config_t *c);
void bt_parse_peer_list(bt_config_t *c);
void bt_dump_config(bt_config_t *c);
bt_peer_t *bt_peer_info(const bt_config_t *c, int peer_id);

void add_peer_list(bt_chunk_list *chunk, bt_peer_t *peer);
void del_peer_list(bt_chunk_list *chunk);
void add_packet_list(bt_chunk_list *chunk, int seq_num, char *data, int data_len);
void del_packet_list(bt_chunk_list *chunk);
void add_receiver_list(bt_config_t *c, char *hash, int id);
void del_receiver_list(bt_config_t* c);
void add_sender_list(bt_config_t *c, char *hash, packet **packets, int num_packets, bt_peer_t *peer);
bt_sender_list *find_sender_list(bt_config_t *c, bt_peer_t *peer);
void del_sender_list(bt_config_t *c, bt_sender_list *sender);

int master_data_file(char *file, bt_config_t *config);
int has_chunk(char *hash, bt_config_t *config);
bt_peer_t *peer_with_addr(struct sockaddr_in *addr, bt_config_t *config);
void reset_peers(bt_config_t *config);

#endif /* _BT_PARSE_H_ */
