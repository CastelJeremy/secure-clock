#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/**
 * Process the format and display time digits.
 *
 * List of available format options:
 * s -> seconds                    -> 0 - 59
 * S -> seconds with leading zero  -> 00 - 59
 * i -> minutes                    -> 0 - 59
 * I -> minutes with leading zero  -> 00 - 59
 * h -> hours                      -> 0 - 23
 * H -> hours with leading zero    -> 00 - 23
 * d -> day                        -> 1 - 31
 * D -> day with leading zero      -> 01 - 31
 * m -> month                      -> 1 - 12
 * M -> month with leading zero    -> 01 - 12
 * y -> year                       -> 2022 - 2038
 * 
 * @format: Requested format.
 */
char* format_str(char* format) {
    time_t* now = malloc(sizeof * now);
    struct tm* tm;
    time(now);
    tm = localtime(now);
    free(now);

    size_t max_size = 128;
    char* res = calloc(max_size, sizeof * res);
    for (size_t i = 0; i < strlen(format); i++) {
        char tmp[12];
        switch (format[i]) {
            case 's':
                snprintf(tmp, sizeof(tmp), "%u", tm->tm_sec);
                break;

            case 'S':
                if (tm->tm_sec < 10) snprintf(tmp, sizeof(tmp), "0%u", tm->tm_sec);
                else snprintf(tmp, sizeof(tmp), "%u", tm->tm_sec);
                break;

            case 'i':
                snprintf(tmp, sizeof(tmp), "%u", tm->tm_min);
                break;

            case 'I':
                if (tm->tm_min < 10) snprintf(tmp, sizeof(tmp), "0%u", tm->tm_min);
                else snprintf(tmp, sizeof(tmp), "%u", tm->tm_min);
                break;

            case 'h':
                snprintf(tmp, sizeof(tmp), "%u", tm->tm_hour);
                break;

            case 'H':
                if (tm->tm_hour < 10) snprintf(tmp, sizeof(tmp), "0%u", tm->tm_hour);
                else snprintf(tmp, sizeof(tmp), "%u", tm->tm_hour);
                break;

            case 'd':
                snprintf(tmp, sizeof(tmp), "%u", tm->tm_mday);
                break;

            case 'D':
                if (tm->tm_mday < 10) snprintf(tmp, sizeof(tmp), "0%u", tm->tm_mday);
                else snprintf(tmp, sizeof(tmp), "%u", tm->tm_mday);
                break;

            case 'm':
                snprintf(tmp, sizeof(tmp), "%u", tm->tm_mon + 1);
                break;

            case 'M':
                if (1 + tm->tm_mon < 10) snprintf(tmp, sizeof(tmp), "0%u", tm->tm_mon + 1);
                else snprintf(tmp, sizeof(tmp), "%u", tm->tm_mon + 1);
                break;

            case 'y':
                snprintf(tmp, sizeof(tmp), "%u", 1900 + tm->tm_year);
                break;

            default:
                tmp[0] = format[i];
                tmp[1] = '\0';
                break;
        }

        if ((strlen(tmp) + strlen(res) + 1) > max_size) {
            max_size = strlen(tmp) + strlen(res) + 1;
            char* big_res = realloc(res, sizeof * big_res * max_size);
            if (big_res == NULL) return res;

            res = big_res;
            for (size_t j = strlen(res); j < max_size; j++) res[j] = '\0';
        }

        strncat(res, tmp, max_size - strlen(res) - 1);
    }

    return res;
}

/**
 * Ensures str never includes the newline character and is always terminated by
 * a null-character.
 * 
 * On failure str is set as an empty string and the function returns -1.
 * 
 * Parse stdin and removes any remaining characters until a newline or the
 * end-of-file is reached.
 * 
 * Removes the newline character and clean the error and EOF indicator of stdin.
 * 
 * @str: Pointer to an array of chars where the string read is copied.
 * @size: Maximum number of characters to be copied into str (including
 * the terminating null-character).
 */
int sfgets(char* str, int size) {
    int res = -1;

    if (fgets(str, size, stdin) != NULL) {
        size_t nl_pos = strcspn(str, "\n");
        if (nl_pos == strlen(str) && !feof(stdin)) {
            int c = '\0';

            while (c != '\n' && c != EOF) {
                c = fgetc(stdin);
            }
        } else if (nl_pos < strlen(str)) {
            str[nl_pos] = '\0';
        }

        res = 0;
    } else {
        str = "\0";
    }

    if (feof(stdin)) clearerr(stdin);
    return res;
}

/**
 * Infinite loop to accept client connections and process an unlimited number
 * of get localtime request.
 * 
 * A request is limited by 64 characters including the null-character.
 * 
 * @arg: File descriptor for the server socket.
 */
void* server(void* arg) {
    int sock = *(int*)arg;

    while (1) {
        int client = accept(sock, NULL, NULL);

        ssize_t s;
        ssize_t write_status;
        do {
            int i = 0;
            char* req = calloc(64, sizeof * req);

            do {
                s = read(client, &req[i], 1);
            } while (s > 0 && req[i++] != '\0' && strlen(req) < 63);

            char* res = format_str(req);
            free(req);

            write_status = write(client, res, strlen(res) + 1);
            free(res);
        } while (s != 0 && write_status != -1);

        close(client);
    }
}

int handleCli() {
    char input[4];
    printf("Method s[et]/g[et]: ");

    if (sfgets(input, 4) == 0) {
        if (strcmp(input, "s") == 0 || strcmp(input, "set") == 0) {
            char time[20];
            printf("Date string `y-m-d h:i:s`: ");

            if (sfgets(time, 20) == 0) {
                struct tm* tm = malloc(sizeof * tm);

                if (strptime(time, "%Y-%m-%d %T", tm)) {
                    time_t t = mktime(tm);
                    free(tm);

                    if (t != -1) {
                        int fork_status = fork();

                        if (fork_status == 0) {
                            char str_time[20];
                            snprintf(str_time, 20, "%ld", t);

                            char* argv[] = { "/usr/bin/settime", str_time, NULL };
                            execvp("/usr/bin/settime", argv);

                            return 0;
                        } else if (fork_status != -1) {
                            int status;
                            wait(&status);

                            if (status == 0) {
                                printf("Date successfully updated\n");
                                return 0;
                            } else {
                                printf("Something went wrong...\n");
                            }
                        } else {
                            printf("Could not fork main process...\n");
                        }

                        return -2;
                    }
                }
            }
        } else if (strcmp(input, "g") == 0 || strcmp(input, "get") == 0) {
            char format[64];
            printf("Format string:\n> ");
            if (sfgets(format, 64) == 0) {
                char* res = format_str(format);

                printf("%s\n", res);
                free(res);
            }

            return 0;
        }
    }

    printf("Invalid input\n");
    return -1;
}

int main() {
    cap_t caps = cap_get_proc();
    cap_value_t required_caps[1] = { CAP_SYS_TIME };

    // Remove every capabilities except required ones
    cap_clear(caps);
    cap_set_flag(caps, CAP_INHERITABLE, 1, required_caps, CAP_SET);
    cap_set_proc(caps);
    cap_free(caps);

    char input[7];
    do {
        printf("Instance c[li]/s[erver]/e[xit]: ");
        if (sfgets(input, 7) == 0) {
            if (strcmp(input, "s") == 0 || strcmp(input, "server") == 0) {
                char str_port[6];
                FILE* conf = fopen("/etc/secureclock.conf", "r");

                if (conf != NULL) {
                    if (fgets(str_port, 6, conf) != NULL) {
                        long long llport = strtol(str_port, NULL, 10);
                        if (llport > 0 && llport <= UINT16_MAX) {

                            uint16_t port = (uint16_t)llport;
                            struct sockaddr_in addr;
                            pthread_t thread;

                            int sock = socket(AF_INET, SOCK_STREAM, 0);
                            addr.sin_family = AF_INET;
                            addr.sin_addr.s_addr = htonl(INADDR_ANY);
                            addr.sin_port = htons(port);

                            if (
                                sock != -1
                                && bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0
                                && listen(sock, 1) == 0
                                && pthread_create(&thread, NULL, server, (void*)&sock) == 0
                            ) {
                                printf("Server running on port %u...\n", port);
                            } else {
                                printf("Could not start server: ");
                                if (errno == EADDRINUSE) printf("Address already in use\n");
                                else if (errno == EACCES) printf("Address is protected\n");
                                else if (errno == EINVAL) printf("Invalid address\n");
                                else printf("Something went wrong\n");

                                close(sock);
                            }
                        } else {
                            printf("Invalid config file");
                        }
                    } else {
                        printf("Could not read config file");
                    }

                    fclose(conf);
                } else {
                    printf("Could not open config file");
                }
            } else if (strcmp(input, "c") == 0 || strcmp(input, "cli") == 0) {
                handleCli();
            } else if (strcmp(input, "e") == 0 || strcmp(input, "exit") == 0) {
                printf("Exiting...\n");
            } else {
                printf("Invalid input\n");
            }
        }
        printf("\n");
    } while (strcmp(input, "e") != 0 && strcmp(input, "exit") != 0);

    return 0;
}
