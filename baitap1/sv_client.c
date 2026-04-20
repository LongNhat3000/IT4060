#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct sinhvien {
    char mssv[16];
    char hoten[64];
    char ngaysinh[16];
    float dtb;
};

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

    // 3. Kết nối
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("connect() failed: %d\n", WSAGetLastError());
        closesocket(client_sock);
        WSACleanup();
        return 1;
    }

    printf("Connected successfully to %s:%d!\n", ip, port);

    while (1) {
        struct sinhvien sv;
        memset(&sv, 0, sizeof(sv));

        printf("\n--- Nhap thong tin sinh vien ---\n");
        printf("MSSV (Nhap '0' de thoat): ");
        scanf("%15s", sv.mssv);

        if (strcmp(sv.mssv, "0") == 0) {
            printf("Dang ngat ket noi...\n");
            break;
        }

        // Xóa bộ nhớ đệm bàn phím
        while(getchar() != '\n'); 

        printf("Ho ten: ");
        fgets(sv.hoten, sizeof(sv.hoten), stdin);
        sv.hoten[strcspn(sv.hoten, "\n")] = 0; 

        printf("Ngay sinh (YYYY-MM-DD): ");
        scanf("%15s", sv.ngaysinh);

        printf("Diem trung binh: ");
        scanf("%f", &sv.dtb);

        // 4. Gửi cả struct đi
        if (send(client_sock, (char*)&sv, sizeof(sv), 0) == SOCKET_ERROR) {
            printf("send() failed: %d\n", WSAGetLastError());
            break; 
        } else {
            printf(">> Da gui thong tin sinh vien [%s] thanh cong!\n", sv.mssv);
        }
    }

    closesocket(client_sock);
    WSACleanup();
    
    return 0;
}
