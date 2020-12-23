#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

int isPaused = 0;
int must_wait;

int cd_cmd(char **args);
int clr_cmd(char **args);
int dir_cmd(char **args);
int environ_cmd(char **args);
int help_cmd(char **args);
int pause_cmd(char **args);
int quit_cmd(char **args);
int my_exec(char **args);

char *builtinStr[] = {
    "cd",
    "clr",
    "dir",
    "environ",
    "help",
    "pause",
    "quit",
};

int (*builtinFunc[])(char **) = {
    &cd_cmd,
    &clr_cmd,
    &dir_cmd,
    &environ_cmd,
    &help_cmd,
    &pause_cmd,
    &quit_cmd};

int builtinsSize()
{
    return sizeof(builtinStr) / sizeof(char *);
}

int cd_cmd(char **args)
{
    if (args[1] == NULL || args[2] != NULL)
        perror("Error: \"cd\" command accepts only one argument!\n");
    else if (chdir(args[1]) != 0)
        perror("Error: error executing \"cd\" command\n");
    return 1;
}

int clr_cmd(char **args)
{
    args[0] = "clear";
    my_exec(args);
    return 1;
}

int dir_cmd(char **args)
{
    args[0] = "ls";
    my_exec(args);
    return 1;
}

int environ_cmd(char **args)
{
    args[0] = "env";
    my_exec(args);
    return 1;
}

int help_cmd(char **args)
{
    args[0] = "man";
    my_exec(args);
    return 1;
}

int pause_cmd(char **args)
{
    printf("MOH@shell:Paused untill you press [Enter]$ ");
    isPaused = 1;
    return 1;
}

int quit_cmd(char **args)
{
    return 0;
}

int exec_status(char **args)
{
    // An empty command was entered.
    if (args[0] == NULL)
        return 1;

    for (int i = 0; i < builtinsSize(); i++)
        if (strcmp(args[0], builtinStr[i]) == 0)
            return (*builtinFunc[i])(args);

    return my_exec(args);
}

char *readLine()
{
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
        {
            exit(EXIT_SUCCESS); // We recieved an EOF - End Of File
        }
        else
        {
            perror("Error: error reading the command\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

#define LSH_TOK_DELIM " \t\r\n\a"
char **splitLine(char *line)
{
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    if (!tokens)
    {
        perror("Error: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += 64;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                free(tokens_backup);
                perror("Error: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int my_exec(char **args)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
            perror("Error: can't run command!");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("Error: can't use fork!");
    }
    else
    {
        if (must_wait)
            wait(NULL);
    }
    return 1;
}

int main(int argc, char **argv)
{
    // Read batch file, if any.
    if (argc > 1)
    {
        FILE *file;
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        file = fopen(argv[1], "r");
        if (file == NULL)
        {
            fprintf(stderr, "Error: can't read file %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        while ((read = getline(&line, &len, file)) != -1)
        {
            char **args = splitLine(line);
            if (exec_status(args) == 0)
                exit(EXIT_SUCCESS);
        }

        fclose(file);
        if (line)
            free(line);
        exit(EXIT_SUCCESS);
    }

    // Run command loop.
    char *line;
    char **args;
    int status;

    do
    {
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        if (!isPaused)
            printf("MOH@shell:%s$ ", cwd);
        
        line = readLine();
        args = splitLine(line);
        
        
        char **arg_pointer = args;
        must_wait = 1;

        while (*arg_pointer) {
            if (strcmp(*arg_pointer, "&") == 0 && *(arg_pointer + 1) == NULL) {
                must_wait = 0;
                *(arg_pointer) = NULL;
            }

            // Change STDIN
            if (strcmp(*arg_pointer, "<") == 0 && *(arg_pointer + 1) != NULL) {

                *(arg_pointer) = NULL;
            }

            // Change STDOUT
            if (strcmp(*arg_pointer, ">") == 0 && *(arg_pointer + 1) != NULL) {

                *(arg_pointer) = NULL;
            }

            // Change STDOUT
            if (strcmp(*arg_pointer, ">>") == 0 && *(arg_pointer + 1) != NULL) {
                *(arg_pointer) = NULL;
            }

            arg_pointer++;
        }

        if (isPaused)
            isPaused = 0;
        else
            status = exec_status(args);

        free(line);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}