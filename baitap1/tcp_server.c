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

    if (argc != 4) {
        printf("Usage: %s <port> <greeting_file> <log_file>\n", argv[0]);
        WSACleanup();
        return 1;
    }

    int port = atoi(argv[1]);
    char *file_chao = argv[2];
    char *file_luu = argv[3];

    // Đọc file câu chào
    FILE *f_hello = fopen(file_chao, "r");
    if (f_hello == NULL) {
        printf("Khong the mo file chao: %s\n", file_chao); 
        WSACleanup();
        return 1;
    }
    
    char hello[1024] = {0};
    fread(hello, 1, sizeof(hello) - 1, f_hello);
    fclose(f_hello);

    // 2. Tạo socket listener
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 3. Bind
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind() failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    // 4. Listen
    if (listen(listener, 5) == SOCKET_ERROR) {
        printf("listen() failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    printf("Waiting for client on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock == INVALID_SOCKET) {
            printf("accept() failed: %d\n", WSAGetLastError());
            continue; 
        }

        printf("Client connected from IP: %s\n", inet_ntoa(client_addr.sin_addr));

        // Gửi câu chào
        send(client_sock, hello, (int)strlen(hello), 0);

        FILE *f_log = fopen(file_luu, "a");
        if (f_log == NULL) {
            printf("Khong the mo file log.\n");
            closesocket(client_sock);
            continue;
        }

        char buffer[1024];
        int bytes_received;

        // Nhận dữ liệu và ghi log
        while ((bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            printf("Received %d bytes: %s", bytes_received, buffer);
            
            fputs(buffer, f_log);
            fflush(f_log); 
        }

        printf("Client disconnected.\n");
        fclose(f_log);
        closesocket(client_sock); 

    closesocket(listener);
    WSACleanup();
    return 0;
}
