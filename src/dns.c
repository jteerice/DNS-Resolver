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
    free(header);
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
        dns_addrs[i] = malloc(INET_ADDRSTRLEN);
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

int create_socket() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "Error: Socket could not be created\n");
        exit(0);
    }
    return sock_fd;
}

static void send_recieve_packet(char** dns_addrs, unsigned char* packet, int packet_len) {
    int sock_fd = create_socket(); 

    /* Create server address struct */
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, dns_addrs[0], &(server_addr.sin_addr));

    /* Connect to dns server */
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "Error: Could not connect to DNS server\n");
        exit(0);
    }

    /* Send it! */
    write(sock_fd, packet, packet_len);
    memset(packet, 0, packet_len);

    if (read(sock_fd, (unsigned char*)packet, PACKET_SIZE) <= 0) {
        fprintf(stderr, "Error: DNS response was not recieved\n");
        exit(0);
    }
}

static void free_dns_servers(char** dns_addrs) {
    for (int i = 0; i < MAX_DNS_ADDRS; i++) {
        free(dns_addrs[i]);
    }
    free(dns_addrs);
}

static void convert_to_dot_format(unsigned char* q_name_addr) {
    unsigned char* ptr = q_name_addr;
    unsigned char* res_ptr = q_name_addr;
    int i = 0;
    while (true) {
        int tmp = ptr[0];
        if (tmp == 0) {
            res_ptr -= 1;
            break;
        }
        ptr += 1;
        for (i = 0; i < tmp; i++) {
            res_ptr[i] = ptr[i];
        }
        res_ptr[i] = '.';
        res_ptr += tmp + 1;
        ptr += tmp;
    }
    *res_ptr = 0x00;
}

static char* parse_response_packet(unsigned char* packet) {
    DNS_HEADER* header = (DNS_HEADER*)&packet;
    int size = sizeof(DNS_HEADER);
    unsigned char* q_name_addr = (unsigned char*)&packet[size];
    convert_to_dot_format(q_name_addr);
    size += strlen((char*)q_name_addr);
    Q_FLAGS* q_flags = (Q_FLAGS*)&packet[size];
    size += sizeof(Q_FLAGS);
    if (header) {}
    if (q_flags) {}
    return "placeholder";
}

void resolve_hostname(const char* hostname) {
    unsigned char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    char** dns_addrs = init_and_retrieve_dns_servers();

    int packet_len = build_packet(packet, hostname);

    send_recieve_packet(dns_addrs, packet, packet_len);
    free_dns_servers(dns_addrs);

    char* ip_address = parse_response_packet(packet);
    if (ip_address) {}
    
}

