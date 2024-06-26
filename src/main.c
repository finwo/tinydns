// @url RFC 1035 https://tools.ietf.org/html/rfc1035

#include "common.h"

#include "cofyc/argparse.h"
#include "finwo/poll.h"

#define VERSION  "0.3.1"

static const char *const usages[] = {
    "tinydns [options]",
    NULL,
};
static const char *const version = "\ntinydns " VERSION "\nAuthor: CupIvan <mail@cupivan.ru>\nLicense: MIT\n";

struct ProxyClient {
  void                *next;
  struct sockaddr_in6  addr;
  socklen_t            len;
  uint16_t             id;
};

// 4KiB buffer, should be plenty as we're dealing with single-packet messages
unsigned char buf[0xFFF];

void error(char *msg) { log_s(msg); perror(msg); exit(1); }

void loop(int sockfd) {

  // Setup main fpoll
  struct fpoll    *pfd = fpoll_create();
  struct fpoll_ev *ev  = malloc(sizeof(struct fpoll_ev));
  fpoll_add(pfd, FPOLL_IN | FPOLL_HUP, sockfd, NULL);

  struct ProxyClient *clients = NULL;
  struct ProxyClient *client;
  struct ProxyClient *client_prev;

  // Processing vars
  int i, n;
  uint16_t  id;
  uint16_t *ans = NULL;

  socklen_t           in_addr_len;
  struct sockaddr_in6 in_addr;

  int                out_socket;
  socklen_t          out_addr_len;
  struct sockaddr_in out_addr;

  // Pre-open udp socket to upstream server
  memset((char *) &out_addr, 0, sizeof(out_addr));
  out_addr.sin_family = AF_INET;
  out_addr.sin_port   = htons(config.upstream_port);
  inet_aton(config.upstream_ip, (struct in_addr *)&out_addr.sin_addr.s_addr);
  out_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (out_socket < 0) error("ERROR opening socket out");
  fpoll_add(pfd, FPOLL_IN | FPOLL_HUP, out_socket, NULL);

  while(1) {
    // TODO: clean expired proxy client queue
    n = fpoll_wait(pfd, ev, 1, 1000);
    if (!n) continue;

    if (ev->fd == sockfd) {
      // Client request

      // receive datagram
      in_addr_len = sizeof(in_addr);
      n = recvfrom(ev->fd, buf, sizeof(buf), 0, (struct sockaddr *) &in_addr, &in_addr_len);
      if (n < 1) continue;

      // clear Additional section, becouse of EDNS: OPTION-CODE=000A add random bytes to the end of the question
      // EDNS: https://tools.ietf.org/html/rfc2671
      THeader* ptr = (THeader*)buf;
      if (ptr->ARCOUNT > 0)
      {
        ptr->ARCOUNT = 0;
        i = sizeof(THeader);
        while (buf[i] && i < n) i += buf[i] + 1;
        n = i + 1 + 4; // COMMENT: don't forget end zero and last 2 words
      }
      // also clear Z: it's strange, but dig util set it in 0x02
      ptr->Z = 0;

      parse_buf((THeader*)buf);

      id = *((uint16_t*)buf);

      log_b("Q-->", buf, n);

      // Search cache for known response
      if ((ans = (uint16_t *)cache_search(buf, (uint16_t *)&n))) {
        ans[0] = id;
        log_b("<--C", ans, n);
      } else {
        cache_question(buf, n);
      }

      if (ans) {
        // Send answer back cached response
        n = sendto(ev->fd, ans, n, 0, (struct sockaddr *) &in_addr, in_addr_len);
        if (n < 0) log_s("ERROR in sendto back");
      } else {
        // Ask parent

        // Add client to queue
        client       = malloc(sizeof(struct ProxyClient));
        client->next = clients;
        client->id   = id;
        client->len  = in_addr_len;
        clients      = client;
        memcpy(&(client->addr), &in_addr, in_addr_len);

        // resend to parent
        out_addr_len = sizeof(out_addr);
        n = sendto(out_socket, buf, n, 0, (struct sockaddr *) &out_addr,  out_addr_len);
        if (n < 0) { log_s("ERROR in sendto");  }
      }

    } else if (ev->fd == out_socket) {
      // TODO: Upstream response

      n = recvfrom(ev->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *) &out_addr, &out_addr_len);
      if (n < 0) continue;

      // Look up client in queue
      id = *((uint16_t*)buf);
      client_prev = NULL;
      client      = clients;
      while(client) {
        if (client->id == id) break;
        client_prev = client;
        client      = client->next;
      }
      if (!client) {
        // Discard
        continue;
      }

      // Remove found client from the list
      if (client_prev) {
        client_prev->next = client->next;
      } else {
        clients = client->next;
      }

      // Add answer to cache
      cache_answer(buf, n);
      log_b("<--P", buf, n);

      // And respond to the client
      in_addr_len = client->len;
      memcpy(&in_addr, &(client->addr), in_addr_len);
      n = sendto(sockfd, buf, n, 0, (struct sockaddr *) &in_addr, in_addr_len);
      if (n < 0) log_s("ERROR in sendto back");

      free(client);
    }
  }
}

#include <netdb.h>
int hostname_to_ip(const char *hostname, char *ip, int len) {
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  if (getaddrinfo(hostname, NULL, &hints, &servinfo) != 0) return 0;

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if (p->ai_family == AF_INET6) {
      struct sockaddr_in6 *serveraddr = (struct sockaddr_in6 *)p->ai_addr;
      inet_ntop(AF_INET6, (struct in_addr *)&serveraddr->sin6_addr, ip, len);
      break;
    } else {
      if (p->ai_family == AF_INET) {
        struct sockaddr_in *serveraddr = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, (struct in_addr *)&serveraddr->sin_addr, ip, len);
      }
    }
  }
  freeaddrinfo(servinfo);
  return 1;
}

int server_init() {
  int sock;

  // convert domain to IP
  char buf[0xFF];
  if (hostname_to_ip(config.bind_ip, buf, sizeof(buf)))
    config.bind_ip = buf;

  // is ipv6?
  int is_ipv6 = 0, i = 0;
  while (config.bind_ip[i]) if (config.bind_ip[i++] == ':') { is_ipv6 = 1; break; }

  // create socket
  sock = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
  * us rerun the server immediately after we kill it;
  * otherwise we have to wait about 20 secs.
  * Eliminates "ERROR on binding: Address already in use" error.
  */
  int optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(optval));

  // bind
  if (is_ipv6) {
    struct sockaddr_in6 serveraddr;  /* server's addr */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin6_family = AF_INET6;
    serveraddr.sin6_port   = htons(config.bind_port);
    inet_pton(AF_INET6, config.bind_ip, (struct in_addr *)&serveraddr.sin6_addr.s6_addr);
    if (bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
      error("ERROR on binding ipv6");
  } else {
    struct sockaddr_in serveraddr;  /* server's addr */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(config.bind_port);
    inet_aton(config.bind_ip, (struct in_addr *)&serveraddr.sin_addr.s_addr);
    if (bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
      error("ERROR on binding ipv4");
  }

  char s[0xFF];
  sprintf(s, "bind on %s#%d", config.bind_ip, config.bind_port);
  log_s(s);

  return sock;
}

int main(int argc, const char **argv) {
  char *config_file = NULL;
  int flag_version  = 0;
  int flag_daemon   = 0;

  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_STRING( 'c', "config" , &config_file , "Config file to load"  , NULL, 0, 0),
    OPT_BOOLEAN('d', "daemon" , &flag_daemon , "Run as daemon"        , NULL, 0, 0),
    OPT_BOOLEAN('v', "version", &flag_version, "Show version and exit", NULL, 0, 0),
    OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argparse_describe(&argparse, "\nSmall DNS server utility with flexible json-based configuration", version);
  argc = argparse_parse(&argparse, argc, argv);

  if (flag_version) {
    printf(version);
    exit(0);
  }

  if (flag_daemon) {
    pid_t pid = fork();
    if (pid < 0) {
      if (pid < 0) error("Can't create daemon!");
      exit(1);
    }
    if (pid > 0) exit(0); // exit from current process
  } else {
    config.debug_level = 1;
  }

  config_load(config_file);

  int sockfd = server_init();

  loop(sockfd);
}
