/* Exam 02 */

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"

#define SENTINEL (char*)NULL /* Sentinel to mark end of Queue */
#define QSIZE 1 << 6         /* Maximum size of Queue structure */

unsigned int Concurrency = 2;
Queue* JobQueue = NULL;
char outdir[PATH_MAX] = {0};

/* Functions */
char* filename_from_url(char* url) {
    char* ptr = url + strlen(url) - 1;
    if (*ptr == '/') {
        *ptr = '\0';
    }

    while (*ptr != '/' && ptr > url) {
        ptr--;
    }

    return ptr + 1;
}

/* Threads */
void* curl_consumer(void* arg) {
    CURL* handle = curl_easy_init();
    if (!handle) exit(EXIT_FAILURE);

    char* url = NULL;

    while ((url = queue_pop(JobQueue))) {
        printf("Starting %s...\n", url);
        char outpath[PATH_MAX];
        char* url_copy = strdup(url);
        char* filename = filename_from_url(url_copy);
        snprintf(outpath, PATH_MAX + 2, "%s/%s", outdir, filename);
        free(url_copy);

        FILE* outfile = fopen(outpath, "w");
        if (!outfile) {
            printf("Failed %s\n", url);
            continue;
        }

        // consume a job from the queue
        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)outfile);
        printf("Finished %s: %d\n", url, curl_easy_perform(handle));

        curl_easy_reset(handle);
        fclose(outfile);
    }

    curl_easy_cleanup(handle);
    return NULL;
}

/* Main Execution */

// To use the easy interface, you must first create yourself an easy handle. You need one handle for each easy session you want to perform. Basically, you should use one handle for ethats very thread you plan to use for transferring. You must never share the same handle in multiple threads.

int main(int argc, char* argv[]) {
    // before we make ANY calls to curl_easy_init, we MUST call curl_global_init to prevent threading bugs
    JobQueue = queue_create(SENTINEL, QSIZE);
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) return EXIT_FAILURE;

    getcwd(outdir, PATH_MAX);
    if (argc < 2) return EXIT_FAILURE;

    int i = 1;
    while (1) {
        if (strcmp(argv[i], "-d") == 0) {
            strncpy(outdir, argv[i + 1], PATH_MAX);
        } else if (strcmp(argv[i], "-n") == 0) {
            Concurrency = atoi(argv[i + 1]);
        } else {
            break;
        }
        i += 2;
    }

    Thread threads[Concurrency];
    for (int i = 0; i < Concurrency; i++) {
        thread_create(&threads[i], NULL, curl_consumer, NULL);
    }

    // Push jobs to queue, followed by sentinels
    for (; i < argc; i++) {
        queue_push(JobQueue, argv[i]);
    }

    for (int i = 0; i < Concurrency; i++) {
        queue_push(JobQueue, SENTINEL);
    }

    size_t failures = 0;
    for (int i = 0; i < Concurrency; i++) {
        size_t result = 0;
        thread_join(threads[i], (void**)&result);
        failures += result;
    }

    curl_global_cleanup();
    queue_delete(JobQueue);

    return failures;
}