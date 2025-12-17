/* Request.c: Request structure */

#include "smq/request.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "smq/utils.h"

/* Internal Structures */

typedef struct {
    char *data;   // Response data string
    size_t size;  // Response data length
} Response;

typedef struct {
    const char *data;  // Payload data string
    size_t offset;     // Payload data offset
} Payload;

/* Internal Functions */

/**
 * Writer function: Copy data up to size*nmemb from ptr to userdata (Response).
 *
 * @param   ptr         Pointer to delivered data.
 * @param   size        Always 1.
 * @param   nmemb       Size of the delivered data.
 * @param   userdata    Pointer to user-provided Response structure.
 **/
size_t request_writer(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = nmemb;
    Response *mem = (Response *)userdata;

    char *response_ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!response_ptr) return 0; /* out of memory */

    mem->data = response_ptr;
    memcpy(&(mem->data[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

/**
 * Reader function: Copy data up to size*nmemb from userdata (Payload) to ptr.
 *
 * @param   ptr         Pointer to data to deliver.
 * @param   size        Always 1.
 * @param   nmemb       Size of the data buffer.
 * @param   userdata    Pointer to user-provided Payload structure.
 **/
size_t request_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
    Payload *pay = (Payload *)userdata;
    if (!pay->data) return 0;
    size_t copy_len = strlen(pay->data);
    if (copy_len - pay->offset <= 0) return 0;

    size_t remaining = copy_len - pay->offset;
    size_t s_nmemb = size * nmemb;
    size_t length = remaining < s_nmemb ? remaining : s_nmemb;

    memcpy(ptr, pay->data + pay->offset, length);
    pay->offset += length;

    return length;
}

/* Functions */

/**
 * Create Request structure.
 * @param   method      Request method string.
 * @param   uri         Request uri string.
 * @param   body        Request body string.
 * @return  Newly allocated Request structure.
 **/
Request *request_create(const char *method, const char *url, const char *body) {
    // Initialize CURL client
    Request *req = calloc(1, sizeof(Request));
    if (req) {
        if (method) {
            req->method = strdup(method);
        }

        if (url) {
            req->url = strdup(url);
        }

        if (body) {
            req->body = strdup(body);
        }

        return req;
    }

    return NULL;
}

/**
 * Delete Request structure.
 * @param   r           Request structure.
 **/
void request_delete(Request *r) {
    if (!r) return;
    free(r->method);
    free(r->url);
    free(r->body);
    free(r);
}

/**
 * Perform HTTP request using libcurl.
 *
 *  1. Initialize curl.
 *  2. Set curl options.
 *  3. Perform curl.
 *  4. Cleanup curl.
 *
 * Note: this must support GET, PUT, and DELETE methods, and adjust options as
 * necessary to support these methods.
 *
 * @param   r           Request structure.
 * @param   timeout     Maximum total HTTP transaction time (in milliseconds).
 * @return  Body of HTTP response (NULL if error or timeout).
 **/
char *request_perform(Request *r, long timeout) {
    if (!r) return NULL;
    // Initialize CURL client
    CURL *curl = curl_easy_init();
    Response res = {0};

    if (curl) {
        // Set CURL options GET/PUT/DELETE
        curl_easy_setopt(curl, CURLOPT_URL, r->url);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, request_writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);

        // Set CURL options PUT
        if (streq(r->method, "PUT")) {
            Payload payload = {.data = r->body, .offset = 0};
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, request_reader);
            curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&payload);
            if (r->body) {
                curl_easy_setopt(curl, CURLOPT_INFILESIZE, (size_t)strlen(r->body));
            } else {
                curl_easy_setopt(curl, CURLOPT_INFILESIZE, (size_t)0);
            }
        }

        // Set CURL options DELETE
        if (streq(r->method, "DELETE")) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        // Perform CURL
        if (curl_easy_perform(curl) != CURLE_OK) {
            curl_easy_cleanup(curl);
            return NULL;
        }

        curl_easy_cleanup(curl);
        return res.data;
    } else {
        fprintf(stderr, "request_perform: failed to initialize libcurl.\n");
        return NULL;
    }
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
