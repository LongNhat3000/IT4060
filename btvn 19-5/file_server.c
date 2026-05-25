#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define SERVER_PORT 5000
#define BUFFER_SIZE 1024

/* Thư mục chứa file download */
#define RES_DIR "/home/longnhat/IT4060_Laptrinhmang/btvn_7_4"

/* ====================================================== */
/* Xử lý zombie process                                   */
/* ====================================================== */
void signal_handler(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* ====================================================== */
/* Gửi toàn bộ dữ liệu                                    */
/* ====================================================== */
int send_all(int sock, const void *buffer, int length)
{
    int total = 0;
    int bytesleft = length;
    int n;

    while (total < length)
    {
        n = send(sock, (char *)buffer + total, bytesleft, 0);

        if (n <= 0)
            return -1;

        total += n;
        bytesleft -= n;
    }

    return total;
}

/* ====================================================== */
/* Gửi danh sách file                                     */
/* ====================================================== */
int send_file_list(int client)
{
    DIR *dir = opendir(RES_DIR);

    if (dir == NULL)
    {
        char *msg = "ERROR Cannot open directory\r\n";
        send_all(client, msg, strlen(msg));
        return -1;
    }

    struct dirent *entry;
    struct stat st;

    char filepath[512];
    char *filenames[256];

    int count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        /* Bỏ . và .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(filepath, sizeof(filepath),
                 "%s/%s", RES_DIR, entry->d_name);

        /* Chỉ lấy file thường */
        if (stat(filepath, &st) == 0 &&
            S_ISREG(st.st_mode))
        {
            if (count >= 256)
                break;

            filenames[count] = strdup(entry->d_name);
            count++;
        }
    }

    closedir(dir);

    if (count == 0)
    {
        char *msg = "ERROR No files available\r\n";
        send_all(client, msg, strlen(msg));
        return -1;
    }

    /* Gửi số lượng file */
    char header[64];

    sprintf(header, "OK %d\r\n", count);
    send_all(client, header, strlen(header));

    /* Gửi tên file */
    for (int i = 0; i < count; i++)
    {
        send_all(client, filenames[i], strlen(filenames[i]));
        send_all(client, "\r\n", 2);

        free(filenames[i]);
    }

    send_all(client, "\r\n", 2);

    printf(">> Sent list of %d files\n", count);

    return 0;
}

/* ====================================================== */
/* Nhận tên file và gửi file                              */
/* ====================================================== */
void receive_and_send_file(int client)
{
    char filename[256];
    char filepath[512];
    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(filename, 0, sizeof(filename));

        int ret = recv(client, filename,
                       sizeof(filename) - 1, 0);

        if (ret <= 0)
        {
            printf("Client disconnected\n");
            break;
        }

        filename[strcspn(filename, "\r\n")] = 0;

        if (strlen(filename) == 0)
            continue;

        /* Chống path traversal */
        if (strstr(filename, "..") != NULL)
        {
            char *msg = "ERROR Invalid filename\r\n";
            send_all(client, msg, strlen(msg));
            continue;
        }

        snprintf(filepath, sizeof(filepath),
                 "%s/%s", RES_DIR, filename);

        FILE *f = fopen(filepath, "rb");

        if (f == NULL)
        {
            char *msg =
                "ERROR File not found. Try again\r\n";

            send_all(client, msg, strlen(msg));

            printf("!! File not found: %s\n", filename);

            continue;
        }

        struct stat st;

        stat(filepath, &st);

        long filesize = st.st_size;

        /* Gửi header */
        char header[64];

        sprintf(header, "OK %ld\r\n", filesize);

        send_all(client, header, strlen(header));

        printf(">> Sending file: %s (%ld bytes)\n",
               filename, filesize);

        int n;

        while ((n = fread(buffer, 1,
                          BUFFER_SIZE, f)) > 0)
        {
            if (send_all(client, buffer, n) < 0)
            {
                printf("Send error\n");
                break;
            }
        }

        fclose(f);

        printf(">> File sent successfully\n");

        break;
    }
}

/* ====================================================== */
/* MAIN                                                   */
/* ====================================================== */
int main()
{
    mkdir(RES_DIR, 0777);

    int listener = socket(AF_INET,
                          SOCK_STREAM,
                          IPPROTO_TCP);

    if (listener < 0)
    {
        perror("socket() failed");
        return 1;
    }

    /* Reuse address */
    setsockopt(listener,
               SOL_SOCKET,
               SO_REUSEADDR,
               &(int){1},
               sizeof(int));

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listener,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5) < 0)
    {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("=====================================\n");
    printf(" File Server is running\n");
    printf(" Port      : %d\n", SERVER_PORT);
    printf(" Resource  : %s\n", RES_DIR);
    printf("=====================================\n");

    signal(SIGCHLD, signal_handler);

    while (1)
    {
        int client = accept(listener, NULL, NULL);

        if (client < 0)
            continue;

        printf(">> New client connected: %d\n", client);

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork() failed");
            close(client);
            continue;
        }

        /* Child process */
        if (pid == 0)
        {
            close(listener);

            if (send_file_list(client) == 0)
            {
                char *msg =
                    "Enter filename to download:\r\n";

                send_all(client, msg, strlen(msg));

                receive_and_send_file(client);
            }

            close(client);

            printf(">> Client disconnected\n");

            exit(0);
        }

        /* Parent process */
        close(client);
    }

    close(listener);

    return 0;
}