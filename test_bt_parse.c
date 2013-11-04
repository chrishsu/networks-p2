#include <stdlib.h>
#include <stdio.h>
#include "bt_parse.h"

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
  add_receiver_list(&config, "1");
  printf("Added Chunk: 1\n");
  add_receiver_list(&config, "2");
  printf("Added Chunk: 2\n");
  add_receiver_list(&config, "3");
  printf("Added Chunk: 3\n");
  add_receiver_list(&config, "4");
  printf("Added Chunk: 4\n");
  add_receiver_list(&config, "5");
  printf("Added Chunk: 5\n");
  bt_chunk_list *cl = config.download;
  while(cl != NULL) {
    printf("Chunk: %s\n", cl->hash);
    cl = cl->next;
  }
  del_receiver_list(&config);
  printf("Del Chunk list\n");
  
  /** Peer List **/
  add_receiver_list(&config, "A");
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
  add_packet_list(config.download, 1, "hi", 2);
  printf("Added Packet: 1\n");
  add_packet_list(config.download, 2, "hi", 2);
  printf("Added Packet: 2\n");
  add_packet_list(config.download, 3, "hi", 2);
  printf("Added Packet: 3\n");
  bt_packet_list *kl = config.download->packets;
  while(kl != NULL) {
    printf("Packet: %d\n", kl->seq_num);
    kl = kl->next;
  }
  
  del_receiver_list(&config);
  printf("Del Chunk list\n");
  
  /** Sender List **/
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