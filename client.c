#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    int sock = 0;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket not created \n");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1514);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Connection failed due to port and ip problems\n");
        return 1;
    }

    while (1) {
        write(sock, argv[1], strlen(argv[1]) + 1);

        int n, i = 0;
        char* res = calloc(256, sizeof * res);
        do {
            n = read(sock, &res[i], 1);
        } while (n > 0 && res[i++] != '\0' && strlen(res) < 256);
        printf("%s\n", res);

        if (n < 0) {
            printf("Standard input error \n");
        }

        sleep(1);
    }

    return 0;
}