#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 10500

// Hàm kiểm tra thông tin đăng nhập từ file login.txt
int check_login(char *user, char *pass) {
    FILE *f = fopen("login.txt", "r");
    if (f == NULL) return 0;
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Telnet Server is listening on port %d...\n", PORT);

    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);
    int max_fd = listener;

    int logged_in[FD_SETSIZE] = {0}; // Mảng đánh dấu trạng thái đăng nhập của từng socket
    char buf[1024];

    while (1) {
        fd_set fdtest = fdread;
        select(max_fd + 1, &fdtest, NULL, NULL, NULL);

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    FD_SET(client, &fdread);
                    if (client > max_fd) max_fd = client;
                    send(client, "Hay nhap user pass de dang nhap: ", 33, 0);
                } else {
                    int ret = recv(i, buf, sizeof(buf) - 1, 0);
                    if (ret <= 0) {
                        FD_CLR(i, &fdread);
                        logged_in[i] = 0;
                        close(i);
                    } else {
                        buf[ret] = 0;
                        buf[strcspn(buf, "\r\n")] = 0; 

                        if (!logged_in[i]) {
                            // Xử lý đăng nhập
                            char user[50], pass[50];
                            if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                                logged_in[i] = 1;
                                send(i, "Dang nhap thanh cong! Nhap lenh:\n", 33, 0);
                            } else {
                                send(i, "Loi dang nhap! Nhap lai user pass: ", 35, 0);
                            }
                        } else {
                            // Xử lý thực thi lệnh hệ thống
                            char cmd[1100], filename[30];
                            // Tạo file kết quả riêng biệt cho mỗi socket để tránh ghi đè dữ liệu
                            sprintf(filename, "out_%d.txt", i); 
                            
                            // Điều hướng kết quả lệnh (stdout) và lỗi (stderr) vào file
                            sprintf(cmd, "%s > %s 2>&1", buf, filename);
                            system(cmd);

                            // Đọc file kết quả và gửi trả lại cho client
                            FILE *f = fopen(filename, "rb");
                            if (f) {
                                while (1) {
                                    int n = fread(buf, 1, sizeof(buf), f);
                                    if (n <= 0) break;
                                    send(i, buf, n, 0);
                                }
                                fclose(f);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
