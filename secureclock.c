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
 *
 * @return:
 *  0 - Successfully read from stdin and inserted into str.
 * -1 - Failed to read from stdin, str is set as empty string.
 * -2 - Successfully read from stdin but reached end-of-file.
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
        str[0] = '\0';
    }

    if (feof(stdin)) {
        clearerr(stdin);
        res = -2;
    }

    return res;
}

/**
 * Split a given string into two different variables. One for the 'command' and
 * the other for the 'argument'.
 *
 * The 'command' and the 'argument' must be separated by a single space
 * character. If there are more than one space character, they will be stored
 * in the argument variable.
 *
 * The original string must not start with a space character.
 *
 * The method uses cmd_limit and arg_limit to check for string overflow and
 * always ensures both cmd and arg ends with a null terminating character.
 *
 * If an error occurs during the process, both cmd and arg variables are fully
 * reset with null characters.
 *
 * @str: Initial string will only be read and splitted into two variables.
 * @cmd: Pointer to an array of chars where the cmd string will be copied.
 * @cmd_limit: Maximum number of elements to be copied into cmd including the
 * null terminating character.
 * @arg: Pointer to an array of chars where the cmd string will be copied.
 * @cmd_limit: Maximum number of elements to be copied into cmd including the
 * null terminating character.
 *
 * @return:
 *  0 - Successfully split str into cmd and arg.
 * -1 - If str is an empty string.
 * -2 - Str starts with a space character.
 * -3 - The 'command' section overflows the cmd_limit size.
 * -4 - The 'argument' section overflows the arg_limit size.
 */
int split_str(char* str, char* cmd, size_t cmd_limit, char* arg, size_t arg_limit) {
    for (size_t i = 0; i < cmd_limit; i++) cmd[i] = '\0';
    for (size_t i = 0; i < arg_limit; i++) arg[i] = '\0';

    if (strlen(str) == 0)
        return -1;

    if (str[0] == ' ')
        return -2;

    size_t cmdi = 0;
    size_t argi = 0;
    for (size_t i = 0; i < strlen(str); i++) {
        if (argi == 0) {
            if ((cmdi < cmd_limit - 1) && (str[i] != ' ')) {
                cmd[cmdi] = str[i];
                cmdi++;
            } else if ((str[i] == ' ') && (i + 1 < strlen(str))) {
                i++;
                arg[argi] = str[i];
                argi++;
            } else {
                for (size_t j = 0; j < cmd_limit; j++) cmd[j] = '\0';
                for (size_t j = 0; j < arg_limit; j++) arg[j] = '\0';
                return -3;
            }
        } else {
            if (argi < arg_limit - 1) {
                arg[argi] = str[i];
                argi++;
            } else {
                for (size_t j = 0; j < cmd_limit; j++) cmd[j] = '\0';
                for (size_t j = 0; j < arg_limit; j++) arg[j] = '\0';
                return -4;
            }
        }
    }

    return 0;
}

int print_help(char* cmd) {
    if (strlen(cmd) > 0) {
        if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
            printf(" > help [ COMMAND ]\n");
            printf(" # Display help for available commands.\n");
            printf(" # COMMAND: optionnal parameter\n");
        } else if (strcmp(cmd, "e") == 0 || strcmp(cmd, "exit") == 0) {
            printf(" > exit\n");
            printf(" # Gracefully leave the process.\n");
        } else if (strcmp(cmd, "g") == 0 || strcmp(cmd, "get") == 0) {
            printf(" > get [ FORMAT ]\n");
            printf(" # Display the local time in a specific format.\n");
            printf(" # FORMAT: optionnal parameter of 64 characters maximum.\n");
            printf(" # (truncate remaining characters)\n");
        } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "set") == 0) {
            printf(" > set DATE TIME\n");
            printf(" # Set the local time.\n");
            printf(" # DATE: required parameter in y/M/D format.\n");
            printf(" # TIME: required parameter in H:I:S format.\n");
        } else if (strcmp(cmd, "u") == 0 || strcmp(cmd, "up") == 0) {
            printf(" > up\n");
            printf(" # Load the port number from /etc/secureclock.conf.\n");
            printf(" # Start the local server.\n");
        } else if (strcmp(cmd, "k") == 0 || strcmp(cmd, "kill") == 0) {
            printf(" > kill\n");
            printf(" # Stop the local server\n.");
        } else {
            printf("  Invalid COMMAND parameter.\n");
        }
    } else {
        printf(" > help : Display the list of commands and arguments description.\n");
        printf(" > exit : Exit the program.\n");
        printf(" > get  : Get the local time in a specific format.\n");
        printf(" > set  : Update the local time.\n");
        printf(" > up   : Start the local server.\n");
        printf(" > kill : Stop the local server.\n");
    }

    return 0;
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

    while (sock != 0) {
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

    return 0;
}

int cmd_get(char* arg) {
    char* res = format_str(arg);

    printf("  %s\n", res);
    free(res);
    return 0;
}

int cmd_set(char* arg) {
    struct tm* tm = malloc(sizeof * tm);

    if (strptime(arg, "%Y-%m-%d %T", tm)) {
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
                    printf("  Date successfully updated.\n");
                    return 0;
                } else {
                    printf("  Something went wrong...\n");
                }
            } else {
                printf("  Could not fork main process...\n");
            }

            return -2;
        }
    } else {
        printf("  Invalid arguments: > help set\n");
    }

    return -1;
}

int cmd_up(int* sock, pthread_t* thread) {
    if (*sock == 0) {
        char str_port[6];
        FILE* conf = fopen("/etc/secureclock.conf", "r");

        if (conf != NULL) {
            if (fgets(str_port, 6, conf) != NULL) {
                long long llport = strtol(str_port, NULL, 10);
                if (llport > 0 && llport <= UINT16_MAX) {

                    uint16_t port = (uint16_t)llport;
                    struct sockaddr_in addr;

                    *sock = socket(AF_INET, SOCK_STREAM, 0);
                    addr.sin_family = AF_INET;
                    addr.sin_addr.s_addr = htonl(INADDR_ANY);
                    addr.sin_port = htons(port);

                    if (
                        *sock != -1
                        && bind(*sock, (struct sockaddr*)&addr, sizeof(addr)) == 0
                        && listen(*sock, 1) == 0
                        && pthread_create(thread, NULL, server, (void*)sock) == 0
                    ) {
                        printf("  Server running on port %u...\n", port);
                        return 0;
                    } else {
                        printf("  Failed to start lcoal server: ");
                        if (errno == EADDRINUSE) printf("address already in use\n");
                        else if (errno == EACCES) printf("address is protected\n");
                        else if (errno == EINVAL) printf("invalid address\n");
                        else printf("something went wrong\n");

                        close(*sock);
                        *sock = 0;

                        return -3;
                    }
                } else {
                    printf("  Invalid config file.\n");
                }
            } else {
                printf("  Could not read config file.\n");
            }

            fclose(conf);
        } else {
            printf("  Could not open config file.\n");
        }

        return -1;
    }

    printf("  Local server is up.\n");
    return -2;
}

int cmd_kill(int* socket, pthread_t* thread) {
    if (*thread != 0) {
        pthread_cancel(*thread);
        *thread = 0;
    }

    if (*socket != 0) {
        close(*socket);
        *socket = 0;

        printf("  Killed local server.\n");
        return 0;
    }

    printf("  Local server is down.\n");
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

    int server_socket = 0;
    pthread_t server_thread = 0;

    printf("\n");
    printf("    ┌─────────────────────┐\n");
    printf("    │ Secure Clock v1.1.0 │\n");
    printf("    └─────────────────────┘\n");
    printf("\n");

    char input[72];
    do {
        printf("§ ");

        int sfgets_status = sfgets(input, 72);
        if (sfgets_status == -2) printf("\n");

        if (strlen(input) > 0) {
            char* cmd = calloc(5, sizeof * cmd);
            char* arg = calloc(65, sizeof * arg);

            split_str(input, cmd, 5, arg, 65);

            if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
                print_help(arg);
            } else if (strcmp(input, "e") == 0 || strcmp(input, "exit") == 0) {
                printf("  Leaving...\n");
            } else if (strcmp(cmd, "g") == 0 || strcmp(cmd, "get") == 0) {
                if (strlen(arg) > 0) {
                    cmd_get(arg);
                } else {
                    cmd_get("H:I:S D/M/y");
                }
            } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "set") == 0) {
                if (strlen(arg) > 0) {
                    cmd_set(arg);
                } else {
                    printf("  Missing arguments: > help set\n");
                }
            } else if (strcmp(cmd, "u") == 0 || strcmp(cmd, "up") == 0) {
                cmd_up(&server_socket, &server_thread);
            } else if (strcmp(cmd, "k") == 0 || strcmp(cmd, "kill") == 0) {
                cmd_kill(&server_socket, &server_thread);
            } else {
                printf("  Unknow command...\n");
            }

            free(cmd);
            free(arg);
        } else {
            printf("  Empty input...\n");
        }

        printf("\n");
    } while (strcmp(input, "e") != 0 && strcmp(input, "exit") != 0);

    return 0;
}
