/* shell.c
 * Demonstration of multiplexing I/O with a background thread also printing.
 **/

#include <ctype.h>
#include <errno.h>
#include <form.h>
#include <ncurses.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "smq/client.h"
#include "smq/crypto.h"
#include "smq/thread.h"

/**
 * TODO:    - Window resizing
 *
 *          - Form on app start
 *          - Get rid of global messages array and assiciated semaphores
 */

/* Constants */

#define BACKSPACE 127
#define MAX_MSG_HISTORY 1000
#define MAX_MSG_LEN BUFSIZ
#define MAX_TIMESTAMP_LEN 50
#define MAX_USERNAME_LEN 50

// This is very hacky, since the client library caps the message body, including the metadata,
// to BUFSIZ, but this frontend allocates more than that.
typedef struct {
    char text[BUFSIZ];
    char timestamp[MAX_TIMESTAMP_LEN];
    char username[MAX_USERNAME_LEN];
} Message;

typedef struct {
    size_t msgCount;
    /**
     * The message number of the bottommost message visible in the messages window.
     */
    size_t offset;
    /**
     * Index indicating the oldest message in the messages window.
     */
    int oldestIndex;
    Message *messages;
    WINDOW *chatWin;
    int winSizeX, winSizeY;
    Mutex mutex;
} Chat;

typedef struct {
    SMQ *smq;
    Chat *chat;
} ThreadArgs;

void chompSpace(char *c) {
    while (*c) {
        if (*c == ' ') {
            *c = '\0';
            break;
        }
        c++;
    }
}

/**
 * Parse command line options.
 * @param   argc        Number of command line options.
 * @param   argv        Array of command line options.
 * @param   s           Pointer to Scheduler structure.
 * @return  return SMQ structure if valid name and host provided else return NULL
 **/
SMQ *startup_screen(int max_x, int max_y) {
    char name[BUFSIZ];
    strcpy(name, getenv("USER"));
    char host[BUFSIZ] = "localhost";
    char port[BUFSIZ] = "9620";

    FIELD *field[3];
    FORM *my_form;
    keypad(stdscr, TRUE);

    int ch;
    /* Initialize the fields */
    field[0] = new_field(1, 50, (max_y / 2) + 2, (max_x / 2) - 25, 0, 0);
    field[1] = new_field(1, 50, (max_y / 2) + 4, (max_x / 2) - 25, 0, 0);
    field[2] = new_field(1, 50, (max_y / 2) + 6, (max_x / 2) - 25, 0, 0);

    set_field_back(field[0], A_UNDERLINE);
    set_field_type(field[0], TYPE_REGEXP, "(localhost) | ([0-9.]*)");
    field_opts_off(field[0], O_AUTOSKIP);
    set_field_back(field[1], A_UNDERLINE);
    set_field_type(field[1], TYPE_INTEGER, 0, 1, 10000);
    field_opts_off(field[1], O_AUTOSKIP);
    set_field_back(field[2], A_UNDERLINE);
    set_field_type(field[2], TYPE_ALPHA, 1);
    field_opts_off(field[2], O_AUTOSKIP);

    /* Create the form and post it */
    my_form = new_form(field);
    post_form(my_form);

    mvprintw((max_y / 2) + 2, (max_x / 2) - 35, "Host:");
    mvprintw((max_y / 2) + 4, (max_x / 2) - 35, "Port:");
    mvprintw((max_y / 2) + 6, (max_x / 2) - 35, "Username:");

    box(stdscr, 0, 0);

    /* Print the logo. */
    char *title[6] = {
        "  _________      .__       .__            __  ",
        " /   _____/____  |__| ____ |  |__ _____ _/  |_ ",
        " \\_____  \\\\__  \\ |  |/ ___\\|  |  \\\\__  \\\\   __\\",
        " /        \\/ __ \\|  \\  \\___|   Y  \\/ __ \\|  |  ",
        "/_______  (____  /__|\\___  >___|  (____  /__| ",
        "        \\/     \\/        \\/     \\/     \\/      "};

    int offsetX = strlen(title[0]) / 2;
    int line = 2;
    box(stdscr, 0, 0);
    for (int i = 0; i < 6; i++) {
        move(line + i, (max_x / 2) - offsetX);
        printw("%s", title[i]);
    }

    move((max_y / 2) + 2, (max_x / 2) - 25);
    refresh();

    /* Loop through to get user requests */
    while ((ch = getch()) != '\n') {
        switch (ch) {
            case KEY_DOWN:
            case '\t':
                /* Go to next field */
                form_driver(my_form, REQ_NEXT_FIELD);
                /* Go to the end of the present buffer */
                /* Leaves nicely at the last character */
                form_driver(my_form, REQ_END_LINE);
                break;
            case KEY_UP:
                /* Go to previous field */
                form_driver(my_form, REQ_PREV_FIELD);
                form_driver(my_form, REQ_END_LINE);
                break;
            case KEY_BACKSPACE:
            case BACKSPACE:
                form_driver(my_form, REQ_DEL_PREV);
            default:
                /* If this is a normal character, it gets */
                /* Printed				  */
                form_driver(my_form, ch);
                break;
        }
    }

    form_driver(my_form, REQ_VALIDATION);
    strcpy(host, field_buffer(field[0], 0));
    strcpy(port, field_buffer(field[1], 0));
    strcpy(name, field_buffer(field[2], 0));
    chompSpace(host);
    chompSpace(port);
    chompSpace(name);

    /* Un post form and free the memory */
    unpost_form(my_form);
    free_field(field[0]);
    free_field(field[1]);
    free_field(field[2]);
    free_form(my_form);

    wclear(stdscr);
    refresh();

    return smq_create(name, host, port);
}

/**
 * Note: Assumes the chat mutex is held.
 * BUG: When the number of messages exceeds MAX_MSG_HISTORY, this function prints the history incorrectly.
 */
void update_message_window(Chat *chat) {
    /* Clear messages window */
    wclear(chat->chatWin);

    /**
     * Paint window with messages at the current offset, starting with
     * the bottommost row (more recent message) and moving upwards (less recent messages).
     */
    size_t messageNum = chat->offset > 0 ? chat->offset - 1 : 0;
    Message *msg;

    for (int i = chat->winSizeY - 2; i >= 1; i--) {
        msg = chat->messages + (messageNum % MAX_MSG_HISTORY);
        wmove(chat->chatWin, i, 1);
        wprintw(chat->chatWin, "[%s] %s: %s", msg->timestamp, msg->username, msg->text);
        if (messageNum <= 0) {
            break;
        }
        messageNum--;
    }

    box(chat->chatWin, 0, 0);
    wrefresh(chat->chatWin);
}

Chat *chat_create(int height, int width) {
    /* Allocate dynamic memory */
    Chat *chat = calloc(1, sizeof(Chat));
    if (!chat) {
        fprintf(stderr, "Calloc failure.\n");
        exit(EXIT_FAILURE);
    }
    chat->messages = calloc(MAX_MSG_HISTORY, sizeof(Message));
    if (!chat->messages) {
        fprintf(stderr, "Calloc failure.\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize syncronization primitives */
    mutex_init(&chat->mutex, NULL);

    /* Initialize ncurses window */
    chat->chatWin = newwin(height, width, 0, 0);
    chat->winSizeX = width;
    chat->winSizeY = height;
    box(chat->chatWin, 0, 0);
    wrefresh(chat->chatWin);

    return chat;
}

void chat_delete(Chat *chat) {
    if (!chat) {
        return;
    }

    if (chat->messages) {
        free(chat->messages);
    }

    delwin(chat->chatWin);

    free(chat);
}

/* Threads */

void *background_thread(void *arg) {
    /* Unpack arguments */
    ThreadArgs *a = (ThreadArgs *)arg;
    SMQ *smq = a->smq;
    Chat *chat = a->chat;

    /* Fetch messages */

    while (smq_running(smq)) {
        char *msg = smq_retrieve(smq);

        if (!msg) {
            break;
        }

        char *timestamp = strtok(msg, ",");
        char *username = strtok(NULL, ",");
        char *text = username + strlen(username) + 1;

        if (!timestamp || !username || !text) {
            fprintf(stderr, "Retrieved malformed message.\n");
            free(msg);
            continue;
        }

        mutex_lock(&chat->mutex);
        Message *curr = chat->messages + (chat->msgCount % MAX_MSG_HISTORY);
        strncpy(curr->timestamp, timestamp, MAX_TIMESTAMP_LEN);
        strncpy(curr->username, username, MAX_TIMESTAMP_LEN);
        strncpy(curr->text, text, MAX_TIMESTAMP_LEN);
        chat->msgCount++;
        /* Move the chat offset if:
            - this message overwrites the offset target.
            - the offset is tracking the most recent message.
        */
        if (chat->offset < chat->msgCount - MAX_MSG_HISTORY || chat->offset == chat->msgCount) {
            chat->offset++;
        }
        update_message_window(chat);
        mutex_unlock(&chat->mutex);
        free(msg);
    }

    return NULL;
}

/* Main Execution */

int main(int argc, char *argv[]) {
    /* Easter egg */
    const unsigned char ciphertext[] = {0x38, 0x0C, 0x5A, 0x9E, 0x88, 0xFD, 0xC1, 0x78, 0x49, 0x18, 0x9B, 0x94, 0xF4, 0x21, 0x80, 0x90, 0xBB, 0x92, 0x31, 0xB9, 0x1E, 0xDB, 0xD2, 0x24, 0xCE, 0x06, 0x10, 0x2B, 0x4A, 0xFC, 0xA3, 0x5D, 0x8E, 0x22, 0xE6, 0x3B, 0x74, 0x59, 0x48, 0x7E, 0xA1, 0x07, 0x70, 0xA5, 0xF6, 0x02, 0x9D, 0x0F};

    /* ncurses Initialization */
    initscr();
    cbreak();
    noecho();

    int max_x = getmaxx(stdscr), max_y = getmaxy(stdscr);
    int chat_height = max_y - 3;

    /* Initialize dat thang */
    Chat *chat = chat_create(chat_height, max_x);
    SMQ *smq = startup_screen(max_x, max_y);
    if (!smq || !chat) return EXIT_FAILURE;
    smq_subscribe(smq, "MSG");

    /* Background Thread */
    Thread background;
    ThreadArgs threadArgs = {.smq = smq, .chat = chat};
    thread_create(&background, NULL, background_thread, (void *)&threadArgs);

    /* Foreground Thread */
    size_t input_index = 0;
    char input_buf[MAX_MSG_LEN / 2] = {0};
    char message[MAX_MSG_LEN] = {0};
    WINDOW *prompt = newwin(3, max_x, chat_height, 0);
    keypad(prompt, TRUE);
    box(prompt, 0, 0);
    bool running = true;
    bool eggTrigger = false;

    while (running) {
        /* Prepare timestamp */
        time_t now = time(NULL);
        struct tm *local_time = localtime(&now);
        char timestamp[MAX_TIMESTAMP_LEN];
        strftime(timestamp, sizeof(timestamp), "%m/%d %H:%M", local_time);

        /* Re-render prompt window */
        wclear(prompt);
        box(prompt, 0, 0);
        wmove(prompt, 1, 1);
        wprintw(prompt, "[%s] %s: %s", timestamp, smq->name, input_buf);
        wrefresh(prompt);

        /* Block until keypress */
        int input_char = wgetch(prompt);

        switch (input_char) {
            case '\n':
                if (strcmp(input_buf, "quit") == 0) {
                    smq_shutdown(smq);
                    running = false;
                    break;
                } else if (strlen(input_buf) > 0) {
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, smq->name, input_buf);
                    smq_publish(smq, "MSG", message);
                }

                if (eggTrigger) {
                    int outLen;
                    unsigned char *hash = hashKey(input_buf, &outLen);
                    unsigned char *decrypted;
                    if (validateKeyHash(hash)) {
                        decrypted = aes256cbc_decrypt(hash, ciphertext, 48, &outLen);
                        snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "Correct.");
                        smq_publish(smq, "MSG", message);
                        sleep(1);
                        snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "The grubhub that you seek is hidden here:");
                        smq_publish(smq, "MSG", message);
                        sleep(1);
                        snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", decrypted);
                        smq_publish(smq, "MSG", message);
                        sleep(1);
                        free(decrypted);
                    } else {
                        snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "WRONG.");
                        smq_publish(smq, "MSG", message);
                    }
                    eggTrigger = false;
                    free(hash);
                }

                if (strcmp(input_buf, "egg") == 0) {
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "Can you solve my riddle?");
                    smq_publish(smq, "MSG", message);
                    sleep(1);
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "Alive without breath, as cold as death;");
                    smq_publish(smq, "MSG", message);
                    sleep(1);
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "Never thirsty, ever drinking,");
                    smq_publish(smq, "MSG", message);
                    sleep(1);
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "All in mail never clinking.");
                    smq_publish(smq, "MSG", message);
                    sleep(1);
                    snprintf(message, MAX_MSG_LEN, "%s,%s,%s", timestamp, "???", "What am I?");
                    smq_publish(smq, "MSG", message);
                    eggTrigger = true;
                }

                input_index = 0;
                input_buf[0] = 0;
                break;
            case KEY_BACKSPACE:
            case BACKSPACE:
            case '\b':
                if (input_index > 0) {
                    input_buf[--input_index] = 0;
                }
                break;
            case KEY_UP:
                /* grab lock, update offset, call messages window rerender */
                mutex_lock(&chat->mutex);
                if (chat->offset > 0) {
                    chat->offset--;
                }
                update_message_window(chat);
                mutex_unlock(&chat->mutex);
                break;
            case KEY_DOWN:
                /* grab lock, update offset, call messages window rerender */
                mutex_lock(&chat->mutex);
                if (chat->msgCount > 0 && chat->offset < chat->msgCount) {
                    chat->offset++;
                }
                update_message_window(chat);
                mutex_unlock(&chat->mutex);
                break;
            default:
                if (!iscntrl(input_char) && input_index < (BUFSIZ / 2 - 1)) {
                    input_buf[input_index++] = input_char;
                    input_buf[input_index] = 0;
                }
                break;
        }
    }

    /* Cleanup */
    endwin();
    thread_join(background, NULL);
    smq_delete(smq);
    chat_delete(chat);

    return 0;
}