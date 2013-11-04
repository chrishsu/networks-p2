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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "udp_utils.h"

#define BT_FILENAME_LEN 255
#define BT_MAX_PEERS 1024

typedef char flag;

typedef struct bt_peer_s {
  short  id;
  flag downloading;
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
  struct bt_packet_list *next;
} bt_packet_list;

typedef struct bt_chunk_list {
  char[20] hash;
  int next_expected;
  bt_peer_list *peers;
  bt_packet_list *packets;
  struct bt_chunk_list *next;
} bt_chunk_list;

typedef struct bt_sender_list {
  char[20] hash;
  packet **packets;
  int num_packets;
  int last_acked;
  int last_sent;
  int window_size;
  flag state; // start or avoid
  char retransmit;
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
  bt_chunk_list *download;
  bt_sender_list *upload;
};
typedef struct bt_config_s bt_config_t;


void bt_init(bt_config_t *c, int argc, char **argv);
void bt_parse_command_line(bt_config_t *c);
void bt_parse_peer_list(bt_config_t *c);
void bt_dump_config(bt_config_t *c);
bt_peer_t *bt_peer_info(const bt_config_t *c, int peer_id);

void add_chunk_list(bt_config_t *c, char *hash);
void add_sender_list(bt_config_t *c, char *hash, packet **packets, int num_packets);

#endif /* _BT_PARSE_H_ */
