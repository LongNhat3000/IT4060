#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 7777
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Cach dung: %s <IP Server>\n", argv[0]);
        return 1;
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    char buf[BUF_SIZE];
    printf("Nhap tin nhan (UDP Echo): ");
    while (fgets(buf, BUF_SIZE, stdin) != NULL) {
        // Gui den Server
        sendto(s, buf, strlen(buf), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Nhan phan hoi tu Server
        int len = recvfrom(s, buf, BUF_SIZE - 1, 0, NULL, NULL);
        if (len > 0) {
            buf[len] = '\0';
            printf("Server echo: %s", buf);
        }
        printf("Nhap tin nhan: ");
    }

    close(s);
    return 0;
}