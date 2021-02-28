#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LENGTH 3000

int out, in, pid, empty;
char rawcommand[MAX_LENGTH], *commands[MAX_LENGTH], *tokens[MAX_LENGTH],
    *infile, *outfile, *alternate_tokens[MAX_LENGTH];

void clear(char* a, char* b) { free(a), free(b); }

void cleanstring(char* a) {
    int i = 0, j = strlen(a) - 1;
    for (int loc = 0; loc <= j; loc++)
        if (isspace(a[loc])) a[loc] = ' ';
    while (a[i] == ' ') i++;
    while (a[j] == ' ' && i <= j) j--;
    for (int loc = i; loc <= j; loc++) a[loc - i] = a[loc];
    a[j - i + 1] = '\0';
    char* b = a;
    while (*a != '\0') {
        while (*a == ' ' && *(a + 1) == ' ') a++;
        *b = *a;
        b++, a++;
    }
    *b = '\0';
}

int runandgetdestination(char* commands, int source, int first, int last) {
    int destination, pipe_nums[2], pip, infile_num, outfile_num,
        m = 1, in = 0, out = 0, flag = 1;
    char* temp = strdup(commands);
    cleanstring(commands);
    tokens[0] = strtok(commands, " ");
    while ((tokens[m] = strtok(NULL, " ")) != NULL) m++;
    tokens[m] = NULL;
    if (tokens[0] != NULL) {
        if (strcmp(tokens[0], "exit") == 0) {
            free(temp);
            exit(0);
            return 0;
        }
        commands = strdup(temp);
        m = 1;
        tokens[0] = strtok(commands, " ");
        while ((tokens[m] = strtok(NULL, " ")) != NULL) m++;
        tokens[m] = NULL;
        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL) {
                chdir(getenv("HOME"));
                clear(temp, commands);
                return 1;
            }
            int check = chdir(tokens[1]);
            if (check < 0) {
                fprintf(stderr, "Can't access %s\n", tokens[1]);
            }
            clear(temp, commands);
            return 1;
        }
    }
    pip = pipe(pipe_nums);
    if (pip < 0) {
        perror("Piping issue here");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        if (source == 0 && first == 1 && last == 0)
            dup2(pipe_nums[1], 1);
        else if (source != 0 && first == 0 && last == 0)
            dup2(source, 0), dup2(pipe_nums[1], 1);
        else
            dup2(source, 0);
        if (strchr(temp, '<') != NULL) in = 1;
        if (strchr(temp, '>') != NULL) out = 1;
        if (in || out) {
            char *toks[MAX_LENGTH], *commands = strdup(temp);
            int m = 1;
            if (in == 1)
                toks[0] = strtok(temp, "<");
            else
                toks[0] = strtok(temp, ">");
            if (out == 1)
                while ((toks[m] = strtok(NULL, ">")) != NULL) m++;
            else
                while ((toks[m] = strtok(NULL, "<")) != NULL) m++;
            if (in && out) {
                cleanstring(toks[1]);
                cleanstring(toks[2]);
                infile = strdup(toks[1]);
                outfile = strdup(toks[2]);
            } else if (in) {
                cleanstring(toks[1]);
                infile = strdup(toks[1]);
            } else if (out) {
                cleanstring(toks[1]);
                outfile = strdup(toks[1]);
            }
            m = 1;
            tokens[0] = strtok(toks[0], " ");
            while ((tokens[m] = strtok(NULL, " ")) != NULL) m++;
            free(commands);
            if (in == 1) {
                infile_num = open(infile, O_RDONLY, 0);
                if (infile_num < 0) {
                    fprintf(stderr, "Can't open %s for reading\n", infile);
                    flag = 0;
                } else {
                    dup2(infile_num, 0);
                    close(infile_num);
                }
            }
            if (out == 1) {
                outfile_num = creat(outfile, 0644);
                if (outfile_num < 0) {
                    fprintf(stderr, "Can't open %s for writing\n", outfile);
                    flag = 0;
                } else {
                    dup2(outfile_num, 1);
                    close(outfile_num);
                }
            }
        }
        if (flag) {
            int execval = execvp(tokens[0], tokens);
            if (execval < 0) {
                int size = strlen(tokens[0]);
                alternate_tokens[0] = malloc((size + 3) * sizeof(char));
                alternate_tokens[0][0] = '.';
                alternate_tokens[0][1] = '/';
                for (int i = 0; i <= size; i++) {
                    alternate_tokens[0][i + 2] = tokens[0][i];
                }
                for (int i = 1; tokens[i] != NULL; i++) {
                    alternate_tokens[i] = tokens[i];
                }
                execval = execvp(alternate_tokens[0], alternate_tokens);
                free(alternate_tokens[0]);
                if (execval < 0) {
                    printf("Can't execute %s\n", tokens[0]);
                }
            }
        }
        clear(infile, outfile);
        exit(0);
    } else
        waitpid(pid, 0, 0);
    if (source) close(source);
    if (last) close(pipe_nums[0]);
    close(pipe_nums[1]);
    destination = pipe_nums[0];
    clear(temp, commands);
    return destination;
}

void runrawcommand() {
    int n = 1;
    commands[0] = strtok(rawcommand, "|");
    while ((commands[n] = strtok(NULL, "|")) != NULL) n++;
    commands[n] = NULL;
    int i = 0, source = 0;
    while (i < n - 1) {
        source = runandgetdestination(commands[i], source, (i == 0), 0);
        ++i;
    }
    runandgetdestination(commands[i], source, (i == 0), 1);
    return;
}

int main() {
    while (1) {
        printf("Shell > ");
        fgets(rawcommand, MAX_LENGTH, stdin);
        empty = 1;
        char* r = rawcommand;
        while (*r != '\0') empty = empty && isspace(*r), r++;
        *(r - 1) = '\0';
        if (empty) continue;
        if (strcmp(rawcommand, "exit") == 0) {
            exit(0);
            clear(infile, outfile);
            return 0;
        }
        pid = 0;
        runrawcommand();
    }
    clear(infile, outfile);
    return 0;
}
