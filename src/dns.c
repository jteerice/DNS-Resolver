#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "../include/dns.h"

static void init_dns_query(unsigned char* packet) {
    DNS_HEADER* header = malloc(sizeof(DNS_HEADER));
    memset(header, 0, sizeof(DNS_HEADER));
    header->identifier = (uint16_t)htons(getpid());
    header->f_c = (uint16_t)(RECURSE_FLAG >> 8);
    header->q_count = (uint16_t)htons(1);
    memcpy(packet, header, sizeof(DNS_HEADER));
}

static void init_q_flags(uint16_t* q_flags) {
    Q_FLAGS flags;
    flags.q_type = htons(HOST_ADDRESSES_FLAG);
    flags.q_class = htons(Q_CLASS_FLAG);
    memcpy(q_flags, &flags, sizeof(Q_FLAGS));
}

static void retrieve_dns_servers(char** dns_addrs) {
    FILE* resolve_conf = fopen("/etc/resolv.conf", "r");
    if (resolve_conf == NULL) {
        fprintf(stderr, "Error: resolve.conf could not be opened\n");
        exit(0);
    }

    char buf[BUF_LEN];
    int i = 0;
    while (i < MAX_DNS_ADDRS) {
        if (feof(resolve_conf) != 0) break;
        fgets(buf, BUF_LEN, resolve_conf);
        if (strncmp(buf, "nameserver", 10) == 0) {
            strcpy(dns_addrs[i], strtok(buf, " "));
            strcpy(dns_addrs[i], strtok(NULL, "\n"));
            i++;
        }
    }

    fclose(resolve_conf);
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
            *res = (char)counter; 
            res += 1;
            strncpy(res, ptr, counter);
            res += strlen(res);
            ptr += (counter + 1);
            res[counter] = 0x00;
            break;
        } else if (ptr[counter] == '.') {
            *res = (char)counter;
            res += 1;
            strncpy(res, ptr, counter);
            res += strlen(res);
            ptr += (counter + 1);
            counter = 0;
        }
        counter++;
    }
}

static char** init_and_retrieve_dns_servers() {
    char **dns_addrs = malloc(MAX_DNS_ADDRS * sizeof(char *));
    for (int i = 0; i < MAX_DNS_ADDRS; i++) {
        *dns_addrs = malloc(INET_ADDRSTRLEN);
    }
    retrieve_dns_servers(dns_addrs);

    return dns_addrs;
}

static int build_packet(unsigned char* packet, const char* hostname) {
    init_dns_query(packet);
    int size = sizeof(DNS_HEADER);
    char* qname_addr = (char*)&packet[sizeof(DNS_HEADER)];
    convert_hostname_to_dns_compatible(qname_addr, (char*)hostname);
    size += strlen(qname_addr);
    uint16_t* q_flags = (uint16_t*)&packet[size + 1];
    init_q_flags(q_flags);
    size += sizeof(Q_FLAGS);

    return size + 1;
}

void resolve_hostname(const char* hostname) {
    unsigned char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    char** dns_addrs = init_and_retrieve_dns_servers();

    int packet_len = build_packet(packet, hostname);



    /* Build socket */
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, dns_addrs[0], &(server_addr.sin_addr));

    /* Connect to dns server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        printf("Error: Could not connect to DNS server\n");
        exit(0);
    }

    /* Send it! */
    write(sock_fd, packet, packet_len);

}

