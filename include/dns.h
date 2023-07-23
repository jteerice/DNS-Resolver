#ifndef DNS_H
#define DNS_H

#define MAX_DNS_ADDRS 10
#define MAX_DNS_ADDR_LEN 16
#define BUF_LEN 100
#define PACKET_SIZE 65535

#define QUERY_TYPE_FLAG 0x8000
#define RECURSE_FLAG 0x400
#define HOST_ADDRESSES 0x0001
#define Q_CLASS 0x0001

struct dns_header {
    uint16_t identifier;
    uint16_t f_c;
    uint16_t q_count;
    uint16_t a_count;
    uint16_t ns_count;
    uint16_t ad_count;
} __attribute__((packed));
typedef struct dns_header DNS_HEADER;

struct dns_q_flags {
    uint16_t q_type;
    uint16_t q_class;
} __attribute__((packed));
typedef struct dns_q_flags Q_FLAGS;

void retrieve_dns_servers(char** dns_addrs);
void resolve_hostname(const char* hostname);

#endif