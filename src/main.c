#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/dns.h"

int main(int argc, char** argv) {

    if (argc != 3) {
        printf("Usage: <exec> <username> <password>\n");
        exit(0);
    }

    resolve_hostname("app.hackthebox.com");

    return 0;
}