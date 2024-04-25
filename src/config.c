#include "common.h"

#include "parson.h"

TConfig config = (TConfig){
  .server_ip     = "127.0.0.1",
  .upstream_ip   = "8.8.8.8",
  .upstream_port = 53,
  .cache_time    = 6*3600,
  .debug_level   = 0,
};

char rr_buf[0xFFF] = {0};

void config_parse_rr(JSON_Object *rr_obj) {
  THeader *rr = (THeader*)rr_buf; rr->QRCOUNT = htons(1);
  size_t entries = json_object_get_count(rr_obj);
  int i, s, x;
  char *dup_name;
  char *dup_buf;
  size_t dup_len;
  char *dup_tok;

  int rr_size, rr_uid = 1;

  for( i=0 ; i<entries ; i++ ) {

    // Turn "*.pizza.calzone.com" into "<1>*<5>pizza<7>calzone<3>com<0>" & copy into rr_buf
    dup_name = strdup(json_object_get_name(rr_obj, i));
    dup_len  = strlen(dup_name);
    dup_buf  = calloc(dup_len + 2, sizeof(char));
    dup_tok = strtok(dup_name, ".");
    while(dup_tok != NULL) {
      dup_buf[strlen(dup_buf)] = (uint8_t)strlen(dup_tok);
      strcat(dup_buf, dup_tok);
      dup_tok = strtok(NULL, ".");
    }
    memcpy((char*)(rr+1), dup_buf, strlen(dup_buf) + 1);

    // Construct question
    rr->uid     = rr_uid++;
    rr->RD      = 1;
    rr->QR      = 0;
    rr->RA      = 0;
    rr->ANCOUNT = 0;
    rr_size = sizeof(THeader) + strlen(dup_buf) + 1;    // header + question domain + question domain terminator
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x01; // TYPE
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x01; // CLASS
    log_b("A-->", rr_buf, rr_size);
    cache_question(rr_buf, rr_size);

    free(dup_name);
    free(dup_buf);

    // Construct anser
    rr->RD      = 1;
    rr->QR      = 1;
    rr->RA      = 1;
    rr->ANCOUNT = htons(1);
    rr_buf[rr_size++] = 0xC0;                           // ??
    rr_buf[rr_size++] = 0x0C;                           // ??
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x01; // TYPE
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x01; // CLASS
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x00; // TTL
    rr_buf[rr_size++] = 0xAA; rr_buf[rr_size++] = 0xAA; // TTL
    rr_buf[rr_size++] = 0x00; rr_buf[rr_size++] = 0x04; // RDLENGTH
    inet_aton(json_object_get_string(rr_obj, json_object_get_name(rr_obj, i)), (struct in_addr *)&rr_buf[rr_size]); rr_size += 4;
    log_b("<--A", rr_buf, rr_size);
    cache_answer(rr_buf, rr_size);

  }

}

void config_parse(JSON_Value *cfg) {
  JSON_Object     *cfg_obj = NULL;
  JSON_Value_Type  cfg_type = json_value_get_type(cfg);

  char *str_tmp;
  char *str_ip;
  char *str_port;
  uint16_t u16_port = 0;

  // Basic type checking
  if (cfg_type != JSONObject) return;
  cfg_obj = json_value_get_object(cfg);

  if (json_object_has_value_of_type(cfg_obj, "server_ip", JSONString)) {
    config.server_ip = strdup(json_object_get_string(cfg_obj, "server_ip"));
  }

  if (json_object_has_value_of_type(cfg_obj, "upstream", JSONString)) {
    str_tmp  = strdup(json_object_get_string(cfg_obj, "upstream"));
    str_ip   = strtok(str_tmp, "#");
    str_port = strtok(NULL   , "#");

    config.upstream_ip = str_ip;

    if (str_port) u16_port = atoi(str_port);
    if (u16_port) config.upstream_port = u16_port;
  }

  if (json_object_has_value_of_type(cfg_obj, "cache_time", JSONNumber)) {
    config.cache_time = (uint32_t)json_object_get_number(cfg_obj, "cache_time");
  }

  if (json_object_has_value_of_type(cfg_obj, "debug_level", JSONNumber)) {
    config.debug_level = (uint8_t)json_object_get_number(cfg_obj, "debug_level");
  }

  if (json_object_has_value_of_type(cfg_obj, "debug_level", JSONNumber)) {
    config.debug_level = (uint8_t)json_object_get_number(cfg_obj, "debug_level");
  }

  if (json_object_has_value_of_type(cfg_obj, "rr", JSONObject)) {
    config_parse_rr(json_object_get_object(cfg_obj, "rr"));
  }

}

void config_load() {
  JSON_Value *cfg = NULL;

  cfg = json_parse_file("tinydns.conf");
  if (!cfg) cfg = json_parse_file("/etc/tinydns.conf");
  if (!cfg) return;

  config_parse(cfg);

  json_value_free(cfg);
}
