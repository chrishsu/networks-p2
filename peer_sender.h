#ifdef _SENDER
#define _SENDER

/**
 * Processes a GET request then sends a DATA packet. 
 */
int process_get(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends DATA packet.
 */
int send_data(int sock, struct sockaddr_in *t, packet *p, bt_config_t *config);

/**
 * Processes ACK with connection_list.
 * Main portion of congestion control (slow start & congestion avoidance).
 */
int process_ack(int sock, struct sockaddr_in *from, packet *p, bt_config_t *config);

#endif