#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "bt_parse.h"

void packet_list(bt_chunk_list *d, int seq) {
  add_packet_list(d, seq, "hi", 2);
  printf("Adding Packet: %d\n", seq);
  printf("Next Expected: %d\n", d->next_expected);
  printf("Total Data: %d\n", (int)d->total_data);
}

int main() {
  bt_config_t config;
  config.download = NULL;
  config.download_tail = NULL;
  config.upload = NULL;
  
  bt_peer_t peer1;
  peer1.id = 1;
  peer1.next = NULL;
  bt_peer_t peer2;
  peer2.id = 2;
  peer2.next = NULL;
  bt_peer_t peer3;
  peer3.id = 3;
  peer3.next = NULL;
  
  /** Chunk List **/
  printf("-------CHUNK LIST-------\n");
  add_receiver_list(&config, "1", 1);
  printf("Added Chunk: 1\n");
  add_receiver_list(&config, "2", 2);
  printf("Added Chunk: 2\n");
  add_receiver_list(&config, "3", 3);
  printf("Added Chunk: 3\n");
  add_receiver_list(&config, "4", 4);
  printf("Added Chunk: 4\n");
  add_receiver_list(&config, "5", 5);
  printf("Added Chunk: 5\n");
  bt_chunk_list *cl = config.download;
  while(cl != NULL) {
    printf("Chunk: %d\n", cl->id);
    cl = cl->next;
  }
  del_receiver_list(&config);
  printf("Del Chunk list\n");
  
  /** Peer List **/
  printf("\n-------PEER LIST-------\n");
  add_receiver_list(&config, "A", 0);
  printf("Added Chunk: A\n");
  add_peer_list(config.download, &peer1);
  printf("Added Peer: peer1\n");
  add_peer_list(config.download, &peer2);
  printf("Added Peer: peer2\n");
  add_peer_list(config.download, &peer3);
  printf("Added Peer: peer3\n");
  bt_peer_list *pl = config.download->peers;
  while (pl != NULL) {
    printf("Peer: %d\n", pl->peer->id);
    pl = pl->next;
  }
  
  /** Packet List **/
  printf("\n-------PACKET LIST-------\n");
  packet_list(config.download, 1);
  packet_list(config.download, 2);
  packet_list(config.download, 4);
  packet_list(config.download, 5);
  packet_list(config.download, 2);
  packet_list(config.download, 4);
  packet_list(config.download, 3);
  bt_packet_list *kl = config.download->packets;
  while(kl != NULL) {
    printf("Packet: %d\n", kl->seq_num);
    kl = kl->next;
  }
  
  del_receiver_list(&config);
  printf("Del Chunk list\n");
  
  /** Sender List **/
  printf("\n-------SENDER LIST-------\n");
  add_sender_list(&config, "1", NULL, 0, &peer1);
  printf("Added Connection: 1\n");
  add_sender_list(&config, "2", NULL, 0, &peer2);
  printf("Added Connection: 2\n");
  add_sender_list(&config, "3", NULL, 0, &peer3);
  printf("Added Connection: 3\n");
  
  bt_sender_list *sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  /** Delete front of list **/
  printf("\n---Delete front of list\n");
  sl = config.upload;
  printf("Del %d\n", sl->peer->id);
  del_sender_list(&config, sl);
  
  sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  add_sender_list(&config, "3", NULL, 0, &peer3);
  printf("Added Connection: 3\n");
  
  sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  /** Delete middle of list **/
  printf("\n---Delete middle of list\n");
  sl = config.upload->next;
  printf("Del %d\n", sl->peer->id);
  del_sender_list(&config, sl);
  
  sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  add_sender_list(&config, "2", NULL, 0, &peer2);
  printf("Added Connection: 2\n");
  
  sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  /** Delete end of list **/
  printf("\n---Delete end of list\n");
  sl = config.upload->next->next;
  printf("Del %d\n", sl->peer->id);
  del_sender_list(&config, sl);
  
  sl = config.upload;
  while (sl != NULL) {
    printf("Connection: %d\n", sl->peer->id);
    sl = sl->next;
  }
  
  /** Delete all of list **/
  sl = config.upload;
  while (sl != NULL) {
    bt_sender_list *slt = sl;
    sl = slt->next;
    del_sender_list(&config, slt);
  }
  printf("Del Sender list\n");
  
  return 0;
}