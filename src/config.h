#ifndef __TINYDNS_CONFIG_H__
#define __TINYDNS_CONFIG_H__

#include <stdlib.h>

typedef struct TConfig
{
  char     *bind_ip;
  char     *upstream_ip;
  uint16_t  bind_port;
  uint16_t  upstream_port;
  uint32_t  cache_time;
  uint8_t   debug_level;
} TConfig;

extern TConfig config;

void config_load(const char *config_file);

#endif // __TINYDNS_CONFIG_H__
