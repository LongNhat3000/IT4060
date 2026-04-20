#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENTS 1000
#define PORT 10600 

int main() {
    // 1. Khởi tạo socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Telnet Server is listening on port %d...\n", PORT);

    struct pollfd fds[MAX_CLIENTS];
    int login[MAX_CLIENTS] = {0}; // Mảng trạng thái login
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[256];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) break;

        // Xử lý kết nối mới
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                login[nfds] = 0;
                nfds++;
                send(client, "Hay nhap user pass de dang nhap:\n", 33, 0);
            } else {
                close(client);
            }
        }

        // Xử lý dữ liệu từ các client
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                ret = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (ret <= 0) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    login[i] = login[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    buf[strcspn(buf, "\r\n")] = 0;

                    // Nếu chưa login -> Kiểm tra user pass
                    if (login[i] == 0) {
                        char user[32], pass[32], tmp[64];
                        if (sscanf(buf, "%s %s", user, pass) == 2) {
                            sprintf(tmp, "%s %s", user, pass);
                            
                            // Kiểm tra file users.txt
                            FILE *f = fopen("users.txt", "r");
                            int found = 0;
                            char line[64];
                            if (f) {
                                while (fgets(line, sizeof(line), f)) {
                                    line[strcspn(line, "\r\n")] = 0;
                                    if (strcmp(line, tmp) == 0) { found = 1; break; }
                                }
                                fclose(f);
                            }
                            if (found) { login[i] = 1; send(fds[i].fd, "Dang nhap thanh cong! Nhap lenh:\n", 33, 0); }
                            else send(fds[i].fd, "Sai user pass! Nhap lai:\n", 25, 0);
                        }
                    } else {
                        // Nếu đã login -> Thực thi lệnh qua system()
                        char cmd[512], filename[32];
                        sprintf(filename, "out_%d.txt", fds[i].fd);
                        sprintf(cmd, "%s > %s 2>&1", buf, filename);
                        system(cmd);

                        FILE *f = fopen(filename, "rb");
                        if (f) {
                            while ((ret = fread(buf, 1, sizeof(buf), f)) > 0)
                                send(fds[i].fd, buf, ret, 0);
                            fclose(f);
                            // remove(filename); // Xóa file tạm sau khi gửi
                        }
                    }
                }
            }
        }
    }
    return 0;
}