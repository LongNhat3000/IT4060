#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

struct sinhvien {
    char mssv[16];
    char hoten[64];
    char ngaysinh[16];
    float dtb;
};

// Hàm lấy thời gian hệ thống định dạng YYYY-MM-DD HH:MM:SS
void get_current_time(char *buffer, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int main(int argc, char *argv[]) {
    // 1. Khởi tạo WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    if (argc != 3) {
        printf("Usage: %s <port> <log_file>\n", argv[0]);
        WSACleanup();
        return 1;
    }

    int port = atoi(argv[1]);
    char *log_file = argv[2];

    // 2. Tạo socket
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

    // 3. Bind & Listen
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind() failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    if (listen(listener, 5) == SOCKET_ERROR) {
        printf("listen() failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock == INVALID_SOCKET) {
            printf("accept() failed: %d\n", WSAGetLastError());
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("\n[+] Client connected from %s\n", client_ip);

        while (1) {
            struct sinhvien sv;
            memset(&sv, 0, sizeof(sv)); 
            
            // Nhận dữ liệu struct từ client
            int bytes_received = recv(client_sock, (char*)&sv, sizeof(sv), 0);

            if (bytes_received <= 0) {
                printf("[-] Client %s disconnected.\n", client_ip);
                break; 
            }

            char time_str[64];
            get_current_time(time_str, sizeof(time_str));

            // In ra màn hình console của Server
            printf("Received: MSSV: %s | Name: %s | DOB: %s | GPA: %.2f\n", 
                   sv.mssv, sv.hoten, sv.ngaysinh, sv.dtb);

            // Ghi vào file log đúng định dạng
            FILE *f_log = fopen(log_file, "a");
            if (f_log == NULL) {
                printf("Error: Could not open log file %s\n", log_file);
            } else {
                fprintf(f_log, "%s %s %s %s %s %.2f\n", 
                        client_ip, time_str, sv.mssv, sv.hoten, sv.ngaysinh, sv.dtb);
                fclose(f_log);
            }
        }

        closesocket(client_sock); 
    }

    closesocket(listener);
    WSACleanup();
    return 0;
}
