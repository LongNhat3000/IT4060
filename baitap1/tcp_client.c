#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> 
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[]) {
    // 1. Khởi tạo WinSock 
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    if (argc != 3) {
        printf("Usage: %s <IP_address> <port>\n", argv[0]);
        WSACleanup();
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]); 

    // 2. Tạo socket 
    SOCKET client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    printf("Connecting to %s:%d...\n", ip, port);
    
    // 3. Kết nối
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("connect() failed: %d\n", WSAGetLastError());
        closesocket(client_sock);
        WSACleanup();
        return 1;
    }

    printf("Connected successfully!\n");

    char buffer[1024];
    while (1) {
        printf("Enter message to send (Type EXIT to quit): ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        send(client_sock, buffer, (int)strlen(buffer), 0);

        // Kiểm tra thoát
        if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("Disconnecting...\n");
            break;
        }
    }
    
    // 4. Đóng socket và dọn dẹp
    closesocket(client_sock); 
    printf("Connection closed.\n");

    return 0;
}
