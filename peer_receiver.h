#ifdef _RECEIVER
#define _RECEIVER

/**
 * Sends a GET request for the given hash.  
 */
int send_get(int sock, struct sockaddr_in *to, char* hash, bt_config_t *config);

/**
 * Adds the data to the master chunk_list, sends an ACK as well.
 */
int process_data(int sock, struct socaddr_in *from, packet *p, bt_config_t *config);

/**
 * Sends an ACK with nextExpected chunk index in the master chunk_list.
 */
int send_ack(int sock, struct sockaddr_in *to, bt_config_t *config);

#endif