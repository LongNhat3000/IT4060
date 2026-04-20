#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#define MAX_CLIENTS 1000
#define PORT 9600 

int main() {
    // Khởi tạo socket lắng nghe (Listener)
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Chat Server is listening on port %d...\n", PORT); 

    // Mảng quản lý các socket bằng poll
    struct pollfd fds[MAX_CLIENTS];
    char *ids[MAX_CLIENTS] = {NULL}; // Lưu tên client sau khi đăng nhập
    int nfds = 1; // Số lượng fd đang được quản lý

    fds[0].fd = listener;
    fds[0].events = POLLIN; // Lắng nghe sự kiện kết nối mới

    char buf[256];

    while (1) {
        int ret = poll(fds, nfds, -1); // Đợi sự kiện xảy ra
        if (ret < 0) break;

        // Nếu socket listener có sự kiện -> có client kết nối mới
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (nfds < MAX_CLIENTS) {
                printf("New client connected: %d\n", client); 
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                ids[nfds] = NULL;
                nfds++;
                char *msg = "Xin chao. Hay dang nhap (client_id: name)!\n";
                send(client, msg, strlen(msg), 0);
            } else {
                close(client);
            }
        }

        // Kiểm tra dữ liệu từ các client đã kết nối
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                ret = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (ret <= 0) {
                    // Client ngắt kết nối, dọn dẹp tài nguyên
                    printf("Client %d disconnected\n", fds[i].fd); 
                    close(fds[i].fd);
                    if (ids[i]) free(ids[i]);
                    
                    // Lấp đầy khoảng trống mảng sau khi xóa
                    fds[i] = fds[nfds - 1];
                    ids[i] = ids[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    if (buf[ret-1] == '\n') buf[ret-1] = 0;
                    
                    // Logic đăng nhập: kiểm tra cú pháp "client_id: name"
                    if (ids[i] == NULL) {
                        char cmd[32], name[32];
                        if (sscanf(buf, "%s %s", cmd, name) == 2 && strcmp(cmd, "client_id:") == 0) {
                            ids[i] = malloc(strlen(name) + 1);
                            strcpy(ids[i], name);
                            send(fds[i].fd, "Dang nhap thanh cong. Hay gui tin nhan.\n", 40, 0);
                        } else {
                            send(fds[i].fd, "Error. Hay nhap dung cu phap 'client_id: name'\n", 47, 0);
                        }
                    } else {
                        // Broadcast tin nhắn cho các client khác
                        time_t now = time(NULL);
                        struct tm *t = localtime(&now);
                        char time_str[32];
                        strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", t);

                        char out_buf[512];
                        sprintf(out_buf, "%s %s: %s\n", time_str, ids[i], buf); 

                        for (int j = 1; j < nfds; j++) {
                            if (ids[j] != NULL && i != j) {
                                send(fds[j].fd, out_buf, strlen(out_buf), 0);
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}
