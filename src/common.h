#ifndef __TINYDNS_COMMON_H__
#define __TINYDNS_COMMON_H__

#include <stdlib.h> // malloc
#include <string.h>
#include <arpa/inet.h> // for inet_aton
#include <unistd.h> // for usleep, fork
#include <stdlib.h> // for exit
#include <stdio.h>

#include "config.h"
#include "parse.h"
#include "cache.h"

void error(char *msg);

void log_s(char *msg);
void log_b(char *prefix, void *ptr, int n);

#endif // __TINYDNS_COMMON_H__
