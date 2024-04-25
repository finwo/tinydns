#include <stdlib.h>

typedef struct TConfig
{
  char     *server_ip;
  char     *upstream_ip;
  uint16_t  upstream_port;
  uint32_t  cache_time;
  uint8_t   debug_level;
} TConfig;

extern TConfig config;

void config_load();
