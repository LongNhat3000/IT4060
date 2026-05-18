#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(12000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }
    listen(listener, 5);

    signal(SIGCHLD, signal_handler);

    printf("Time Server listening on port 12000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        
        printf("New client connected: %d\n", client);

        if (fork() == 0) {
            close(listener);
            char *welcome_msg = "Chao ban! Day la Time Server.\n"
                                "Hay nhap lenh theo cu phap: GET_TIME [format]\n"
                                "Cac dinh dang ho tro:\n"
                                "- dd/mm/yyyy\n"
                                "- dd/mm/yy\n"
                                "- mm/dd/yyyy\n"
                                "- mm/dd/yy\n";
            send(client, welcome_msg, strlen(welcome_msg), 0);

            char buf[256];
            while (1) {
                int len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) break;

                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                // Nếu client chỉ bấm Enter (chuỗi rỗng), bỏ qua không xử lý 
                if (strlen(buf) == 0) continue;

                printf("Received from client %d: %s\n", client, buf);

                char cmd[16], format[32], tmp[16];
                // Thêm giới hạn ký tự rộng (%15s, %31s) để chống tràn bộ đệm
                int n = sscanf(buf, "%15s %31s %15s", cmd, format, tmp);

                if (n != 2 || strcmp(cmd, "GET_TIME") != 0) {
                    char *err = "Error: Sai cu phap. Yeu cau: GET_TIME [format]\n";
                    send(client, err, strlen(err), 0);
                    continue;
                }

                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char res[64];
                int valid_format = 1;

                if (strcmp(format, "dd/mm/yyyy") == 0) {
                    strftime(res, sizeof(res), "%d/%m/%Y\n", t);
                } else if (strcmp(format, "dd/mm/yy") == 0) {
                    strftime(res, sizeof(res), "%d/%m/%y\n", t);
                } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                    strftime(res, sizeof(res), "%m/%d/%Y\n", t);
                } else if (strcmp(format, "mm/dd/yy") == 0) {
                    strftime(res, sizeof(res), "%m/%d/%y\n", t);
                } else {
                    valid_format = 0;
                }

                if (valid_format) {
                    send(client, res, strlen(res), 0);
                } else {
                    char *err = "Error: Dinh dang thoi gian khong duoc ho tro.\n";
                    send(client, err, strlen(err), 0);
                }
            }

            printf("Client %d disconnected. Child process %d exiting.\n", client, getpid());
            close(client);
            exit(0); 
        }

        close(client);
    }

    close(listener);
    return 0;
}