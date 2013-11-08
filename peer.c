/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "udp_utils.h"
#include "queue.h"
#include "peer_receiver.h"
#include "peer_sender.h"

#define WAIT_MILLIS 100
#define CC_TIMEOUT 5.0 //5 seconds

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 10; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  bt_parse_command_line(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif

  reset_peers(&config);
  peer_run(&config);
  return 0;
}


void process_inbound_udp(int sock, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "process_inbound_udp\n");
  #define BUFLEN 1500
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];

  fromlen = sizeof(from);
  short bytes_read = spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);
  if (bytes_read <= 0 ||
      bytes_read < (short)sizeof(packet_head)) {
    fprintf(stderr, "Error reading in process_inbound_udp!\n");
    return;
  }

  DPRINTF(DEBUG_INIT, "Read packet!\n");
  packet p;
  memcpy(&p.header, buf, sizeof(packet_head));
  if (ntohs(p.header.packet_len) != bytes_read) {
    fprintf(stderr, "Packet has wrong packet_len!\n");
    return;
  }
  if (ntohs(p.header.magic_num) != 15441 ||
      p.header.version != 1) {
    fprintf(stderr, "Packet has bad magic_num or version\n");
    return;
  }

  p.buf = malloc(ntohs(p.header.packet_len) - ntohs(p.header.header_len));
  memcpy(p.buf, buf + ntohs(p.header.header_len), ntohs(p.header.packet_len) - ntohs(p.header.header_len));

  DPRINTF(DEBUG_INIT, "Incoming message from %s:%d\n",
	 inet_ntoa(from.sin_addr),
	 ntohs(from.sin_port));

  DPRINTF(DEBUG_INIT, "\n\nSwitching: %d\n", (int)p.header.type);
  switch(p.header.type) {
  case TYPE_WHOHAS:
    process_whohas(sock, &from, &p, config);
    break;
  case TYPE_IHAVE:
    process_ihave(sock, &from, &p, config);
    break;
  case TYPE_GET:
    process_get(sock, &from, &p, config);
    break;
  case TYPE_DATA:
    process_data(sock, &from, &p, config);
    break;
  case TYPE_ACK:
    process_ack(sock, &from, &p, config);
    break;
  case DROPPED:
    DPRINTF(DEBUG_INIT, "DROPPED!\n");
    break;
  default:
    DPRINTF(DEBUG_INIT, "TYPE: %d\n", p.header.type);
    // Not yet implemented
    break;
  }
  free(p.buf);
}

void process_user_get(int sock, char *chunkfile, char *outputfile, bt_config_t *config) {
  DPRINTF(DEBUG_INIT, "Process User GET (%s, %s)\n", chunkfile, outputfile);

  FILE *CFILE;
  CFILE = fopen(chunkfile, "r");
  if (CFILE == NULL) {
    fprintf(stderr, "No chunkfile!\n");
    return;
  }
  int id;
  char hash_text[41];
  uint8_t hash[20];

  reset_peers(config);
  strcpy(config->output_file, outputfile);
  config->num_chunks = 0;
  config->cur_download = 0;
  config->num_downloaded = 0;
  strcpy(config->get_chunk_file, chunkfile);
  while (fscanf(CFILE, "%d %s", &id, hash_text) == 2) {
    hex2binary(hash_text, 40, hash);
    add_receiver_list(config, (char *)hash, id);
  }

  send_whohas(sock, config);
}

void handle_user_input(int sock, char *line, bt_config_t *config) {
  char chunkf[128], outf[128];

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      process_user_get(sock, chunkf, outf, config);
    }
    else {
      printf("Usage: GET [chunk-file] [out-file]\n\n");
    }
  }
}

void peer_packet_ops(int sock) {
  packet_queue *pq = packet_pop();
  if (pq == NULL) return;
  DPRINTF(DEBUG_INIT, "Trying to send %d bytes...\t", (int)(pq->len));

  int bytes_sent = spiffy_sendto(sock, pq->buf, pq->len, 0, (struct sockaddr*)pq->dest_addr, sizeof(*(pq->dest_addr)));
  DPRINTF(DEBUG_INIT, "sent %d bytes\n", (int)bytes_sent);
  //printf("here %d!\n", (int)bytes_sent);
  if (bytes_sent < 0) {
    fprintf(stderr, "Error sending packet: %d\n", errno);
    fprintf(stderr, "EINTrR = %d, EINVAL = %d\n", EINTR, EINVAL);
  } else {
    size_t new_len = pq->len - bytes_sent;
    if (new_len > 0) {
      memcpy(pq->buf, pq->buf + bytes_sent, new_len);
      pq->len = new_len;
      packet_push(pq);
    } else {
      packet_free(pq);
    }
  }
}

/**
 * @param sender Sender
 * @param start Sequence number to start at
 */
void packet_sender(bt_sender_list *sender, int start) {
  DPRINTF(DEBUG_INIT, "packet_sender: %d\n", start);
  if (sender->packets == NULL) {
    DPRINTF(DEBUG_INIT, "sender->packets empty :(\n");
    return;
  }

  int i;
  int send_up_to = start + sender->window_size - 1;
  if (send_up_to > sender->num_packets)
    send_up_to = sender->num_packets;
  // Start at sending the next packet
  // Sequence numbers go from start to send_up_to - 1
  for (i = start - 1; i < send_up_to; i++) {
    if (sender->packets[i] == NULL) {
      DPRINTF(DEBUG_INIT, "sender->packets[%d] empty :(", i);
      return;
    }
    //printf("\tAdding #%d\n", ntohl(sender->packets[i]->header.seq_num));
    packet_new(sender->packets[i], &(sender->peer->addr));
  }
  sender->sent_time = time(NULL);
  sender->last_sent = send_up_to;
}

/** Congestion Control **/
void peer_cc(bt_config_t *config) {

  bt_sender_list *sender = config->upload;
  /* Iterate through connections */
  while (sender != NULL) {
    // if we have sent everything and been acked, cleanup
    if (sender->num_packets == sender->last_acked) {
      DPRINTF(DEBUG_INIT, "Finished sending!\n");
      bt_sender_list *tmp = sender->next;
      del_sender_list(config, sender);
      sender = tmp;
      continue;
    }

    int cc = 0;
    // lost packet
    if (sender->retransmit >= 3) {
      DPRINTF(DEBUG_INIT, "Retransmitting %d\n", sender->last_acked);
      sender->retransmit = 0;
      cc = 1;
    }

    // timeout
    if (difftime(time(NULL), sender->sent_time) > CC_TIMEOUT && sender->sent_time != 0) {
      DPRINTF(DEBUG_INIT, "Timeout on %d\n", sender->last_acked);
      sender->dropped++;
      cc = 1;
    }

    if (sender->dropped > 3) {
      DPRINTF(DEBUG_INIT, "Peer is dead!\n");
      bt_sender_list *tmp = sender->next;
      del_sender_list(config, sender);
      sender = tmp;
      continue;
    }

    // update window size
    if (cc) {
      if (sender->state == 1) sender->state = 0;
      sender->ssthresh = sender->window_size/2;
      if (sender->ssthresh < 2) sender->ssthresh = 2;
      sender->window_size = 1;
      packet_sender(sender, sender->last_acked);
      continue;
    }

    // queue up packets up to window size
    if (sender->last_acked == sender->last_sent) {
      packet_sender(sender, sender->last_acked + 1);
    }

    sender = sender->next;
  }
}

void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readfds, writefds;
  struct user_iobuf *userbuf;

  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  inet_aton("127.0.0.1", (struct in_addr *)&myaddr.sin_addr.s_addr);
  //myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);

  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

  packet_init();
  FD_ZERO(&writefds);
  FD_ZERO(&readfds);

  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);

    struct timeval waittime;
    waittime.tv_sec = 0;
    waittime.tv_usec = WAIT_MILLIS * 1000;

    if (!packet_empty()) {
      FD_SET(sock, &writefds);
    }

    nfds = select(sock+1, &readfds, &writefds, NULL, &waittime);

    timeout_check(sock, config);
    get_all_the_thingz(sock, config);

    if (nfds > 0) {
      if (FD_ISSET(sock, &writefds)) {
        peer_packet_ops(sock);
        FD_CLR(sock, &writefds);
      }

      if (FD_ISSET(sock, &readfds)) {
        DPRINTF(DEBUG_INIT, "Got packet!\n\n");
        process_inbound_udp(sock, config);
      }

      if (FD_ISSET(STDIN_FILENO, &readfds)) {
        process_user_input(STDIN_FILENO, userbuf, handle_user_input, sock, config);
      }
    }

    peer_cc(config);
  }
}
