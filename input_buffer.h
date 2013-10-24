#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "bt_parse.h"

#ifndef _INPUT_BUFFER
#define _INPUT_BUFFER

#define USERBUF_SIZE 8191

struct user_iobuf {
  char *buf;
  unsigned int cur;
};

struct user_iobuf *create_userbuf();

void process_user_input(int fd, struct user_iobuf *userbuf,
			void (*handle_line)(int, char *, bt_config_t *),
			int sock, bt_config_t *cbdata);

#endif
