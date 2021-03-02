
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define ERROR_FILE    0
#define REG_FILE      1
#define DIRECTORY     2

#define HEADER_BUFFER_SIZE 1024
#define ERROR_RESPONSE_SIZE 1024

void start_provisioning_server() {
   short            http_port = 8001;
   const char      *http_addr = "127.0.0.1";
   struct evhttp *http_server = NULL;

   // don't exit on broken pipe (just fail with message)
   signal(SIGPIPE, brokenPipe);

   // new event base for async stuff
   struct event_base *base = event_base_new();

   // create a new libevent http server
   http_server = evhttp_new(base);
   // bind to a particular address and port (can be specified via argvs)
   evhttp_bind_socket(http_server, http_addr, http_port);
   // set jsonRequestHandler as a callback for all requests (no others are caught specifically)
   evhttp_set_gencb(http_server, jsonRequestHandler, (void *)base);

   fprintf(stderr, "Server started on %s port %d\n", http_addr, http_port);
   event_base_dispatch(base);

   //evhttp_free(http_server);
   //event_base_free(base);
   
   //fprintf(stderr, "Server Died\n");

   while(1){

   }
}

void brokenPipe(int signum) {
   fprintf(stderr, "Broken Pipe (%d)\n", signum);
}

void jsonRequestHandler(struct evhttp_request *request, void *arg) {
   struct event_base *base = (struct event_base *)arg;

   // Request
   struct evbuffer *requestBuffer;
   size_t requestLen;
   char *requestDataBuffer;

   json_t *requestJSON;
   json_error_t error;

   // Error buffer
   char errorText[ERROR_RESPONSE_SIZE];

   // Process Request
   requestBuffer = evhttp_request_get_input_buffer(request);
   requestLen = evbuffer_get_length(requestBuffer);

   requestDataBuffer = (char *)malloc(sizeof(char) * requestLen);
   memset(requestDataBuffer, 0, requestLen);
   evbuffer_copyout(requestBuffer, requestDataBuffer, requestLen);

   printf("%s\n", evhttp_request_uri(request));

   requestJSON = json_loadb(requestDataBuffer, requestLen, 0, &error);
   free(requestDataBuffer);

   if (requestJSON == NULL) {
      snprintf(errorText, ERROR_RESPONSE_SIZE, "Input error: on line %d: %s\n", error.line, error.text);
      sendErrorResponse(request, errorText);
   } else {
      // Debug out
      requestDataBuffer = json_dumps(requestJSON, JSON_INDENT(3));
      printf("%s\n", requestDataBuffer);
      free(requestDataBuffer);

      sendJSONResponse(request, requestJSON, base);
      json_decref(requestJSON);
   }
   return;
}

void sendErrorResponse(struct evhttp_request *request, char *errorText) {
   // Reponse
   char responseHeader[HEADER_BUFFER_SIZE];
   size_t responseLen;

   struct evbuffer *responseBuffer;

   responseLen = strlen(errorText);

   responseBuffer = evbuffer_new();

   // content length to string
   sprintf(responseHeader, "%d", (int)responseLen);

   evhttp_add_header(request->output_headers, "Content-Type", "text/plain");
   evhttp_add_header(request->output_headers, "Content-Length", responseHeader);

   evbuffer_add(responseBuffer, errorText, responseLen);

   evhttp_send_reply(request, 400, "Bad JSON", responseBuffer); 

   evbuffer_free(responseBuffer);
}

void sendJSONResponse(struct evhttp_request *request, json_t *requestJSON, struct event_base *base) {
   // Reponse
   json_t *responseJSON;
   json_t *message;

   char responseHeader[HEADER_BUFFER_SIZE];

   char *responseData;
   int responseLen;
   struct evbuffer *responseBuffer;

   // Create JSON response data
   responseJSON = json_object();

   message = json_string("Hello World");
   json_object_set_new(responseJSON, "message", message);

   // dump JSON to buffer and store response length as string
   responseData = json_dumps(responseJSON, JSON_INDENT(3));
   responseLen = strlen(responseData);
   snprintf(responseHeader, HEADER_BUFFER_SIZE, "%d", (int)responseLen);
   json_decref(responseJSON);

   // create a response buffer to send reply
   responseBuffer = evbuffer_new();

   // add appropriate headers
   evhttp_add_header(request->output_headers, "Content-Type", "application/json");
   evhttp_add_header(request->output_headers, "Content-Length", responseHeader);

   evbuffer_add(responseBuffer, responseData, responseLen);

   // send the reply
   evhttp_send_reply(request, 200, "OK", responseBuffer); 

   evbuffer_free(responseBuffer);
}
