#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "../include/dns.h"

static DNS_HEADER* init_dns_query() {
    DNS_HEADER* header = malloc(sizeof(DNS_HEADER));
    memset(header, 0, sizeof(DNS_HEADER));
    header->identifier = (uint16_t)htons(getpid());
    header->f_c = (uint16_t)htons(QUERY_TYPE_FLAG);
    header->f_c |= (uint16_t)htons(RECURSE_FLAG);
    header->q_count = (uint16_t)1;

    return header; 
}

Q_FLAGS* init_q_flags() {
    Q_FLAGS* q_flags = malloc(sizeof(Q_FLAGS));
    memset(q_flags, 0, sizeof(Q_FLAGS));
    q_flags->q_type = HOST_ADDRESSES;
    q_flags->q_class = Q_CLASS;

    return q_flags;
}

static void convert_hostname_to_dns_compatible(char* qname_addr, char* hostname) {
    /* Need to convert hostname 
       From: www.hackthebox.com
       To: 3www10hackthebox3com0 */
    char* res = qname_addr;
    char* ptr = hostname;
    int counter = 0;
    while (true) {
        if (ptr[counter] == 0) {
            sprintf(res, "%d", counter);
            res += strlen(res);
            strncpy(res, ptr, counter);
            res += strlen(res);
            ptr += (counter + 1);
            res[counter] = 0x00;
            break;
        } else if (ptr[counter] == '.') {
            sprintf(res, "%d", counter);
            res += strlen(res);
            strncpy(res, ptr, counter);
            res += strlen(res);
            ptr += (counter + 1);
            counter = 0;
        }
        counter++;
    }
}

void resolve_hostname(const char* hostname) {
    unsigned char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    /* Create and populate char** array to store string representation of dns server addresses to query */
    char **dns_addrs = malloc(MAX_DNS_ADDRS * sizeof(char *));
    for (int i = 0; i < MAX_DNS_ADDRS; i++) {
        *dns_addrs = malloc(INET_ADDRSTRLEN);
    }
    retrieve_dns_servers(dns_addrs);

    /* Initialize dns packet header */
    DNS_HEADER* header = init_dns_query();
    memcpy((void*)packet, (void*)header, sizeof(header));
    char* qname_addr = (char*)&packet[sizeof(header)];
    convert_hostname_to_dns_compatible(qname_addr, (char*)hostname);
    printf("%s\n", qname_addr);
    Q_FLAGS* q_flags = init_q_flags();
    if(q_flags) {}
}

void retrieve_dns_servers(char** dns_addrs) {
    FILE* resolve_conf = fopen("/etc/resolv.conf", "rt");
    if (resolve_conf == NULL) {
        fprintf(stderr, "Error: resolve.conf could not be opened\n");
        exit(0);
    }

    char buf[BUF_LEN];
    for (int i = 0; i < MAX_DNS_ADDRS; i++) {
        fgets(buf, BUF_LEN, resolve_conf);
        if (strncmp(buf, "nameserver", 10) == 0) {
            strcpy(dns_addrs[i], strtok(buf, " "));
            strcpy(dns_addrs[i], strtok(NULL, "\n"));
        }
    }

    fclose(resolve_conf);
}