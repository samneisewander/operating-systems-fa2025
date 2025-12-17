/* unit_request.c: Test SMQ Request structure (Unit) */

#include "smq/request.h"
#include "smq/utils.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Type Definitions */

typedef struct {
    char * url;
    size_t size;
} TestCase;

/* Constants */

const TestCase GOOD_URLS[] = {
    {"http://weasel.h4x0r.space:9898/", 325},
    {"http://weasel.h4x0r.space:9898/txt/gatsby.txt", 306227},
    {NULL, 0},
};

const TestCase BAD_URLS[] = {
    {"http://neverssl.com/404", 0},
    {"http://fake.host.lol:206/", 0},
    {"http://weasel.h4x0r.space:206/", 0},
    {NULL, 0},
};

char *URL  = "http://weasel.h4x0r.space:9910/";
char *BODY = "genie in a bottle";

/* Functions */

int test_00_request_create() {
    Request *r0 = request_create("GET", URL, BODY);
    assert(r0);
    assert(streq(r0->method, "GET"));
    assert(streq(r0->url   , URL));
    assert(streq(r0->body  , BODY));
    free(r0->method);
    free(r0->url);
    free(r0->body);
    free(r0);

    Request *r1 = request_create("GET", URL, NULL);
    assert(r1);
    assert(streq(r1->method, "GET"));
    assert(streq(r1->url, URL));
    assert(r1->body == NULL);
    free(r1->method);
    free(r1->url);
    free(r1);

    Request *r2 = request_create("GET", NULL, NULL);
    assert(r2);
    assert(streq(r2->method, "GET"));
    assert(r2->url  == NULL);
    assert(r2->body == NULL);
    free(r2->method);
    free(r2);

    Request *r3 = request_create(NULL, NULL, NULL);
    assert(r3);
    assert(r3->method == NULL);
    assert(r3->url    == NULL);
    assert(r3->body   == NULL);
    free(r3);

    return EXIT_SUCCESS;
}

int test_01_request_delete() {
    Request *r0 = request_create("GET", URL, BODY);
    assert(r0);
    assert(streq(r0->method, "GET"));
    assert(streq(r0->url   , URL));
    assert(streq(r0->body  , BODY));
    request_delete(r0);

    Request *r1 = request_create("GET", URL, NULL);
    assert(r1);
    assert(streq(r1->method, "GET"));
    assert(streq(r1->url, URL));
    assert(r1->body == NULL);
    request_delete(r1);

    Request *r2 = request_create("GET", NULL, NULL);
    assert(r2);
    assert(streq(r2->method, "GET"));
    assert(r2->url  == NULL);
    assert(r2->body == NULL);
    request_delete(r2);

    Request *r3 = request_create(NULL, NULL, NULL);
    assert(r3);
    assert(r3->method == NULL);
    assert(r3->url    == NULL);
    assert(r3->body   == NULL);
    request_delete(r3);

    return EXIT_SUCCESS;
}

int test_02_request_perform_get() {
    // Test successful GETs
    for (const TestCase *tc = GOOD_URLS; tc->url; tc++) {
        Request request  = {"GET", tc->url, NULL};
        char   *response = request_perform(&request, 2500);
        assert(response);
        assert(strlen(response) == tc->size);
        free(response);

        // Test timeout
        assert(request_perform(&request, 1) == NULL);
    }

    // Test bad host and port
    for (const TestCase *tc = BAD_URLS; tc->url; tc++) {
        Request request = {"GET", tc->url, NULL};
        assert(request_perform(&request, 1000) == NULL);
    }

    return EXIT_SUCCESS;
}

int test_03_request_perform_put() {
    // Test successful GETs and PUTs
    for (const TestCase *tc = GOOD_URLS; tc->url; tc++) {
        Request download = {"GET", tc->url, NULL};
        char   *payload  = request_perform(&download, 2500);
        assert(payload);
        assert(strlen(payload) == tc->size);

        Request upload   = {"PUT", URL, payload};
        char   *response = request_perform(&upload, 5000);
        assert(response);
        assert(strlen(response) == tc->size);
        assert(streq(response, payload));
        free(response);

        // Test timeout
        assert(request_perform(&upload, 1) == NULL);
        free(payload);
    }

    // Test bad host, bad port, empty body
    for (const TestCase *tc = &BAD_URLS[1]; tc->url; tc++) {
        Request request = {"PUT", tc->url, NULL};
        assert(request_perform(&request, 1000) == NULL);
    }

    return EXIT_SUCCESS;
}

int test_04_request_perform_delete() {
    // Test successful DELETE
    Request request = {"DELETE", "http://weasel.h4x0r.space:9910/a/b/c", NULL};
    char *response = request_perform(&request, 1000);
    assert(response);
    assert(atoi(response) == 6);
    free(response);

    // Test timeout
    assert(request_perform(&request, 1) == NULL);

    // Test bad host, bad port
    for (const TestCase *tc = BAD_URLS; tc->url; tc++) {
        Request request = {"DELETE", tc->url, NULL};
        assert(request_perform(&request, 1000) == NULL);
    }

    return EXIT_SUCCESS;
}

/* Main execution */

int main(int argc, char *argv[]) {
    if (argc != 2) {
	fprintf(stderr, "Usage: %s NUMBER\n\n", argv[0]);
	fprintf(stderr, "Where NUMBER is right of the following:\n");
	fprintf(stderr, "    0. Test request_create\n");
	fprintf(stderr, "    1. Test request_delete\n");
	fprintf(stderr, "    2. Test request_perform_(get)\n");
	fprintf(stderr, "    3. Test request_perform_(put)\n");
	fprintf(stderr, "    4. Test request_perform_(delete)\n");
	return EXIT_FAILURE;
    }

    int number = atoi(argv[1]);
    int status = EXIT_FAILURE;

    switch (number) {
	case 0:  status = test_00_request_create(); break;
	case 1:  status = test_01_request_delete(); break;
	case 2:  status = test_02_request_perform_get(); break;
	case 3:  status = test_03_request_perform_put(); break;
	case 4:  status = test_04_request_perform_delete(); break;
	default: fprintf(stderr, "Unknown NUMBER: %d\n", number); break;
    }

    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
