#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 7777
#define BUF_SIZE 1024

int main() {
    int s = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM cho UDP
    if (s < 0) { perror("Loi socket"); return 1; }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Loi bind");
        return 1;
    }

    printf("UDP Echo Server dang chay tai port %d...\n", PORT);

    char buf[BUF_SIZE];
    while (1) {
        // Nhan du lieu tu bat ky client nao
        int len = recvfrom(s, buf, BUF_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
        if (len <= 0) continue;
        buf[len] = '\0';

        printf("Nhan tu [%s:%d]: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);

        // Gui tra lai (Echo) chinh xau vua nhan
        sendto(s, buf, len, 0, (struct sockaddr*)&client_addr, client_len);
    }

    close(s);
    return 0;
}