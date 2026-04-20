#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define PORT 9500

int main() {
    // Tạo socket lắng nghe với giao thức TCP
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }
    listen(listener, 5);

    printf("Chat Server is listening on port %d...\n", PORT);

    fd_set fdread;     
    FD_ZERO(&fdread);   
    FD_SET(listener, &fdread); 
    int max_fd = listener;    

    char client_names[FD_SETSIZE][50]; 
    memset(client_names, 0, sizeof(client_names));

    char buf[1024];

    while (1) {
        fd_set fdtest = fdread; 
        if (select(max_fd + 1, &fdtest, NULL, NULL, NULL) < 0) break;

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &fdtest)) { // Kiểm tra socket i có sự kiện (dữ liệu đến) không
                if (i == listener) {
                    // Chấp nhận kết nối mới
                    int client = accept(listener, NULL, NULL);
                    FD_SET(client, &fdread);
                    if (client > max_fd) max_fd = client; // Cập nhật max_fd
                    
                    char *msg = "Hay gui ten theo cu phap 'client_id: client_name'\n";
                    send(client, msg, strlen(msg), 0);
                } else {
                    // Nhận dữ liệu từ client hiện tại
                    int ret = recv(i, buf, sizeof(buf) - 1, 0);
                    if (ret <= 0) {
                        // Client ngắt kết nối
                        FD_CLR(i, &fdread);
                        memset(client_names[i], 0, 50);
                        close(i);
                    } else {
                        buf[ret] = 0;
                        buf[strcspn(buf, "\r\n")] = 0;

                        if (client_names[i][0] == 0) {
                            // Xử lý đăng ký tên nếu client chưa có tên
                            char name[50];
                            if (sscanf(buf, "client_id: %49s", name) == 1) {
                                strcpy(client_names[i], name);
                                send(i, "Dang nhap thanh cong!\n", 22, 0);
                            } else {
                                char *err = "Sai cu phap! Yeu cau: 'client_id: client_name'\n";
                                send(i, err, strlen(err), 0);
                            }
                        } else {
                            // Xử lý gửi tin nhắn
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            char time_buf[30];
                            strftime(time_buf, sizeof(time_buf), "%Y/%m/%d %I:%M:%S%p", t);

                            char out_buf[2048];
                            sprintf(out_buf, "%s %s: %s\n", time_buf, client_names[i], buf);

                            // Gửi cho tất cả client khác đã đăng nhập
                            for (int j = 0; j <= max_fd; j++) {
                                if (FD_ISSET(j, &fdread) && j != listener && j != i && client_names[j][0] != 0) {
                                    send(j, out_buf, strlen(out_buf), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
