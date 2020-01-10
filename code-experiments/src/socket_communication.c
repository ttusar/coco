#include <stdio.h>
#include <string.h>
#include "coco_platform.h"

#define HOST "127.0.0.1"    /* Local host */
#define MESSAGE_SIZE 8192   /* Large enough for the entire message */
#define RESPONSE_SIZE 1024  /* Large enough for the entire response (objective values or constraint violations) */

/**
 * @brief Data type needed for socket communication (used by the suites that need it).
 */
typedef struct {
  unsigned short port;       /**< @brief The port for communication with the external evaluator. */
  char *host_name;           /**< @brief The host name for communication with the external evaluator. */
  int precision_x;           /**< @brief Precision used to write the x-values to the external evaluator. */
} socket_communication_data_t;

/**
 * @brief Frees the memory of a socket_communication_data_t object.
 */
static void socket_communication_data_free(void *stuff) {

  socket_communication_data_t *data;

  assert(stuff != NULL);
  data = (socket_communication_data_t *) stuff;
  if (data->host_name != NULL) {
    coco_free_memory(data->host_name);
  }
}

static socket_communication_data_t *socket_communication_data_initialize(
    const char *suite_options, const unsigned short default_port) {

  socket_communication_data_t *data;
  data = (socket_communication_data_t *) coco_allocate_memory(sizeof(*data));

  data->host_name = coco_strdup(HOST);
  if (coco_options_read_string(suite_options, "host_name", data->host_name) == 0) {
    strcpy(data->host_name, HOST);
  }

  data->port = default_port;
  if (coco_options_read(suite_options, "port", "%hu", &(data->port)) == 0) {
  }

  data->precision_x = 8;
  if (coco_options_read_int(suite_options, "precision_x", &(data->precision_x)) != 0) {
    if ((data->precision_x < 1) || (data->precision_x > 32)) {
      data->precision_x = 8;
      coco_warning("socket_communication_data_initialize(): Adjusted precision_x value to %d",
          data->precision_x);
    }
  }

  return data;
}

/**
 * Creates the message for the evaluator. The message has the following format:
 * s <s> t <t> r <r> f <f> i <i> d <d> x <x1> <x2> ... <xd>
 * Where
 * <s> is the suite name (for example, "toy-socket")
 * <t> is the type of evaluation (one of "objectives", "constraints", "add_info")
 * <r> is the number of values to be returned
 * <f> is the function number
 * <i> is the instance number
 * <d> is the problem dimension
 * <xi> is the i-th value of x (there should be exactly d x-values)
 */
static char *socket_communication_create_message(char *message,
                                                 const char *suite_name,
                                                 const char *evaluation_type,
                                                 const size_t number_of_values,
                                                 const size_t function,
                                                 const size_t instance,
                                                 const size_t dimension,
                                                 const double *x,
                                                 const int precision_x) {
  size_t i;
  int write_count, offset;

  offset = sprintf(message, "s %s t %s r %lu f %lu i %lu d %lu x ",
      suite_name, evaluation_type, number_of_values, function, instance, dimension);
  for (i = 0; i < dimension; i++) {
    write_count = sprintf(message + offset, "%.*e ", precision_x, x[i]);
    offset += write_count;
  }
  return message;
}

/**
 * Reads the evaluator response and saves it into values. The response should have
 * the following format:
 * <v1> ... <vn>
  * Where
 * <vi> is the i-th value
 * n is the expected number of values
 */
static void socket_communication_save_response(const char *response,
                                               const long response_size,
                                               const size_t expected_number_of_values,
                                               double *values) {
  size_t i;
  int read_count, offset;

  if (response_size < 1) {
    coco_error("socket_communication_save_response(): Incorrect response %s (size %d)",
        response, response_size);
  }

  offset = 0;
  for (i = 0; i < expected_number_of_values; i++) {
    if (sscanf(response + offset, "%lf%*c%n", &values[i], &read_count) != 1) {
      fprintf(stderr, "socket_communication_save_response(): Failed to read response %s at %s",
          response, response + offset);
      exit(EXIT_FAILURE);
    }
    offset += read_count;
  }
}

/**
 * Sends the message to the external evaluator through sockets. The external evaluator must be running
 * a server using the same port.
 *
 * Should be working for different platforms.
 */
static void socket_communication_evaluate(const char* host_name,
                                          const unsigned short port,
                                          const char *message,
                                          const size_t expected_number_of_values,
                                          double *values) {
  char response[RESPONSE_SIZE];

#if WINSOCK
  WSADATA wsa;
  SOCKET sock;
  SOCKADDR_IN serv_addr;
  int response_len;

  /* Initialize Winsock */
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    coco_error("socket_communication_evaluate(): Winsock initialization failed: %d", WSAGetLastError());
  }

  /* Create a socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    coco_error("socket_communication_evaluate(): Could not create socket: %d", WSAGetLastError());
  }

  serv_addr.sin_addr.s_addr = inet_addr(host_name);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  /* Connect to the evaluator */
  if (connect(sock, (SOCKADDR *) &serv_addr, sizeof(serv_addr)) < 0) {
    /* Wait a bit and try again */
    coco_sleep_ms(300);
    if (connect(sock, (SOCKADDR *) &serv_addr, sizeof(serv_addr)) < 0)
      coco_error("socket_communication_evaluate(): Connection failed (host = %s, port = %d)\nIs the server running?\nMessage: %s",
          host_name, port, message);
  }

  /* Send message */
  if (send(sock, message, (int)strlen(message) + 1, 0) < 0) {
    coco_error("socket_communication_evaluate(): Send failed: %d", WSAGetLastError());
  }
  coco_debug("Sent message: %s", message);

  /* Receive the response */
  if ((response_len = recv(sock, response, RESPONSE_SIZE, 0)) == SOCKET_ERROR) {
    coco_error("socket_communication_evaluate(): Receive failed: %d", WSAGetLastError());
  }
  coco_debug("Received response: %s (length %d)", response, response_len);

  socket_communication_save_response(response, response_len, expected_number_of_values, values);

  closesocket(sock);
  WSACleanup();
#else
  int sock;
  struct sockaddr_in serv_addr;
  long response_len;

  /* Create a socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    coco_error("socket_communication_evaluate(): Socket creation error");
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  /* Convert IPv4 and IPv6 addresses from text to binary form */
  if (inet_pton(AF_INET, host_name, &serv_addr.sin_addr) <= 0) {
    coco_error("socket_communication_evaluate(): Invalid address / Address not supported");
  }

  /* Connect to the evaluator */
  if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    coco_error("socket_communication_evaluate(): Connection failed (host = %s, port = %d)\nIs the server running?",
        host_name, port);
  }

  /* Send message */
  if (send(sock, message, strlen(message) + 1, 0) < 0) {
    coco_error("socket_communication_evaluate(): Send failed");
  }
  coco_debug("Sent message: %s", message);

  /* Receive the response */
  response_len = read(sock, response, RESPONSE_SIZE);
  coco_debug("Received response: %s (length %ld)", response, response_len);

  socket_communication_save_response(response, response_len, expected_number_of_values, values);

  close(sock);
#endif
}

/**
 * @brief Calls the external evaluator to evaluate the objective values for x.
 */
static void socket_evaluate(coco_problem_t *problem, const double *x, double *y) {

  char message[MESSAGE_SIZE];
  const char evaluation_type[] = "objectives";
  socket_communication_data_t *data = (socket_communication_data_t *) problem->suite->data;

  socket_communication_create_message(
      message,
      problem->suite->suite_name,
      evaluation_type,
      problem->number_of_objectives,
      problem->suite_dep_function,
      problem->suite_dep_instance,
      problem->number_of_variables,
      x,
      data->precision_x
  );
  socket_communication_evaluate(
      data->host_name,
      data->port,
      message,
      problem->number_of_objectives,
      y
  );
}

