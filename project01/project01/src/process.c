/* process.c: PQSH Process */

#include "pqsh/utils.h"
#include "pqsh/process.h"
#include "pqsh/timestamp.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/**
 * Create new process structure given command.
 * @param   command     String with command to execute.
 * @return  Pointer to new process structure
 **/
Process *process_create(const char *command) {
    
    // 1. Allocate memory for the process struct and handle failure
    Process *p = calloc(1, sizeof(Process));
    if (!p) {
        fprintf(stderr, "process_create: Failed to allocate memory for process struct. (errno: %s)\n", strerror(errno));
        return NULL;
    }

    // 2. Initialize the struct properties and return

    // NOTE: Our Google overlords have demanded that we use snprintf instead of strcpy
    //       to avoid a buffer overflow.
    snprintf(p->command, BUFSIZ, "%s", command);
    p->arrival_time = timestamp();

    return p;
}

/**
 * Start process by forking and executing the command.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not starting the process was successful
 **/
bool process_start(Process *p) {

    if (!p) {
        return false;
    }

    // Allocate memory on stack for command argument string array
    char *argv[MAX_ARGUMENTS] = {0};

    // Need to copy command over because strtok modifies the source string
    char command_copy[BUFSIZ];
    strcpy(command_copy, p->command);

    int i = 0;

    // Tokenize the command and save in string array
    for (char *token = strtok(command_copy, " "); token; token = strtok(NULL, " ")) {
        argv[i++] = token;
    }

    argv[i] = NULL;

    // Fork
    pid_t pid = fork();

    switch (pid) {
        case 0: // Child: Exec and Exit
            execvp(argv[0], argv);
            fprintf(stderr, "process_start: Child failed to execute command. (errno: %s)\n", strerror(errno));
            exit(EXIT_FAILURE);
            break;
        case -1: // Failure
            fprintf(stderr, "process_start: Failed to fork process. (errno %s)\n", strerror(errno));
            return false;
            break;
        default: // Parent: Update child pid in process struct
            p->start_time = timestamp();
            p->pid = pid;
            return true;
    }

}

/**
 * Pause process by sending it the appropriate signal.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not sending the signal was successful.
 **/
bool process_pause(Process *p) {

    if (!p || !p->pid) {
        fprintf(stderr, "process_pause: Attempt to pause a process that was never started.\n");
        return false;
    }

    if (kill(p->pid, SIGSTOP) < 0) {
        fprintf(stderr, "process_pause: Failed to pause process. (errno %d)\n", errno);
        return false;
    } else {
        return true;
    }
    
}

/**
 * Resume process by sending it the appropriate signal.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not sending the signal was successful.
 **/
bool process_resume(Process *p) {

    if (!p || !p->pid) {
        fprintf(stderr, "process_resume: Attempt to resume a process that was never started.\n");
        return false;
    }

    if (kill(p->pid, SIGCONT) < 0) {
        fprintf(stderr, "process_resume: Failed to resume process. (errno %d)\n", errno);
        return false;
    } else {
        return true;
    }
    
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
