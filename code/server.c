/*
 * server.c
 * ----------------------------------------
 * HTTP Server for Stepper Motor Control
 *
 * Description:
 * This file implements a lightweight HTTP server using the lwIP stack,
 * enabling remote configuration and monitoring of a stepper motor system
 * through a web browser or client application.
 *
 * The server supports two main endpoints:
 *
 * 1. GET /getParams
 *    - Returns the current motor parameters in JSON format.
 *    - Useful for remote monitoring or debugging.
 *
 * 2. GET /setParams?rs=...&ra=...&rd=...&cis=...&fis=...&sm=...&dt=...
 *    - Parses and updates the motor configuration based on the provided
 *      query parameters:
 *        - rs  = rotational speed
 *        - ra  = rotational acceleration
 *        - rd  = rotational deceleration
 *        - cis = current position in steps
 *        - fis = final position in steps
 *        - sm  = step mode
 *        - dt  = dwell time at the final position
 *    - Updates are sent via queue to the motor control task.
 *    - Returns the new configuration as a JSON object.
 *
 * Components:
 * - server_application_thread: Main socket loop handling requests.
 * - process_query_string: Parses the query string and updates the parameters.
 * - parse_query_parameter: Processes each key-value pair individually.
 * - write_to_socket: Sends HTTP responses to the client.
 * - print_pars: Helper function for logging current motor parameters.
 *
 * Notes:
 * - Designed for embedded FreeRTOS-based systems using lwIP and Xilinx SDK.
 */


 #include "server.h"


 /* Main server application thread */
 void server_application_thread()
 {
     int sock, new_sd;
     int size, n;
     struct sockaddr_in address, remote;
     char recv_buf[RECV_BUF_SIZE];
     char http_response[1024];
 
     memset(&address, 0, sizeof(address));
 
     // Create new socket
     if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         xil_printf("Error creating socket.\r\n");
         return;
     }
 
     // Address settings
     address.sin_family = AF_INET;
     address.sin_port = htons(SERVER_PORT);
     address.sin_addr.s_addr = INADDR_ANY;
 
     // Bind socket to address
     if (lwip_bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
         xil_printf("Error on lwip_bind.\r\n");
         return;
     }
 
     // Start listening
     lwip_listen(sock, 0);
     size = sizeof(remote);
 
     struct pollfd fds[1];
     fds[0].fd = sock;
     fds[0].events = POLLIN;
 
     while (1) {
         int ret = poll(fds, 1, 10);
         if (ret > 0) {
             new_sd = lwip_accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size);
             if (new_sd < 0) {
                 xil_printf("Error accepting connection.\r\n");
                 continue;
             }
 
             memset(recv_buf, 0, sizeof(recv_buf));
             n = read(new_sd, recv_buf, RECV_BUF_SIZE - 1);
             if (n < 0) {
                 xil_printf("Error reading from socket %d, closing.\r\n", new_sd);
                 close(new_sd);
                 continue;
             }
 
             recv_buf[n] = '\0';  // Null-terminate
             // 1) GET /getParams
             if (strncmp(recv_buf, "GET /getParams", 14) == 0) {
             /* --------------------------------------------------*/
                 // TODO 2: Grab real-time data from the motor driver
 
                 motor_pars.current_position = stepper_get_pos();
                 motor_pars.rotational_speed = stepper_get_speed();
 
                 int computed_direction = (motor_pars.rotational_speed >= 0) ? 1 : 0;
 
 
                 snprintf(http_response, sizeof(http_response),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Connection: close\r\n\r\n"
                         "{"
                         "\"rotational_speed\": %.2f,"
                         "\"rotational_accel\": %.2f,"
                         "\"rotational_decel\": %.2f,"
                         "\"current_position\": %ld,"
                         "\"final_position\": %lu,"
                         "\"step_mode\": %d,"
                         "\"dwell_time\": %d,"
                         "\"direction\": %d"
                         "}",
                         motor_pars.rotational_speed,
                         motor_pars.rotational_accel,
                         motor_pars.rotational_decel,
                         motor_pars.current_position,
                         motor_pars.final_position,
                         motor_pars.step_mode,
                         motor_pars.dwell_time,
                         computed_direction);
             /* --------------------------------------------------*/
 
             /* 2) GET /setParams
              * e.g. /setParams?rs=500&ra=100&rd=100&cis=0&fis=4096&sm=1
              */
             } else if (strncmp(recv_buf, "GET /setParams", 14) == 0) {
                 // Parse query string to update motor_pars
                 process_query_string(recv_buf, &motor_pars);
 
                 // Send updated parameters to motor queue
                 xQueueSend(motor_queue, &motor_pars, 0);
             /* --------------------------------------------------*/
                 // TODO 1: Return updated parameters as JSON
                 snprintf(http_response, sizeof(http_response),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Connection: close\r\n\r\n"
                          "{"
                            "\"rotational_speed\": %.2f,"
                            "\"rotational_accel\": %.2f,"
                            "\"rotational_decel\": %.2f,"
                            "\"current_position\": %ld,"
                            "\"final_position\": %lu,"
                            "\"step_mode\": %d,"
                            "\"dwell_time\": %d"
                          "}",
                          motor_pars.rotational_speed,
                          motor_pars.rotational_accel,
                          motor_pars.rotational_decel,
                          motor_pars.current_position,
                          motor_pars.final_position,
                          motor_pars.step_mode,
                          motor_pars.dwell_time);
             /* --------------------------------------------------*/
 
             } else {
                 // Return 404 for any other request
                 snprintf(http_response, sizeof(http_response),
                          "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: application/json\r\n"
                          "Connection: close\r\n\r\n"
                          "{\"error\": \"Unknown endpoint\"}");
             }
             // Send the response
             write_to_socket(new_sd, http_response);
             close(new_sd);
         }
     }
 }
 
 
 /* Helper function to write to socket */
 int write_to_socket(int sd, const char* buffer)
 {
     int nwrote = write(sd, buffer, strlen(buffer));
     if (nwrote < 0) {
         xil_printf("ERROR responding to client. tried = %d, written = %d\r\n",
                    (int)strlen(buffer), nwrote);
         xil_printf("Closing socket %d\r\n", sd);
     }
     return nwrote;
 }
 
 
 /* Process query string: parse name/value pairs into motor_parameters_t */
 void process_query_string(const char* query, motor_parameters_t* params)
 {
     char name[64];
     char value[64];
     const char* params_start = strchr(query, '?');
 
     if (!params_start) {
         xil_printf("No query parameters found.\n");
         return;
     }
     params_start++; // move past '?'
 
     while (1) {
         int bytesRead;
         if (sscanf(params_start, "%63[^=]=%63[^& ]%n", name, value, &bytesRead) == 2) {
             parse_query_parameter(name, value, params);
             params_start += bytesRead;
             // Skip '&'
             if (*params_start == '&')
                 params_start++;
             if (*params_start == '\0')
                 break; // End of string
         } else {
             break; // No more valid pairs
         }
     }
 }
 
 
 /* Parse individual name/value pairs into the motor parameters */
 int parse_query_parameter(const char* name, const char* value, motor_parameters_t* params)
 {
     int recognized = 1;
 
     if (strcmp(name, "rs") == 0) {
         float speed = atof(value);
         if (speed < 0) {
             xil_printf("Invalid rotational speed: %.2f. Setting to 0.\r\n", speed);
             speed = 0;
         }
         params->rotational_speed = speed;
 
     } else if (strcmp(name, "ra") == 0) {
         float accel = atof(value);
         if (accel < 0) {
             xil_printf("Invalid rotational acceleration: %.2f. Setting to 0.\r\n", accel);
             accel = 0;
         }
         params->rotational_accel = accel;
 
     } else if (strcmp(name, "rd") == 0) {
         float decel = atof(value);
         if (decel < 0) {
             xil_printf("Invalid rotational deceleration: %.2f. Setting to 0.\r\n", decel);
             decel = 0;
         }
         params->rotational_decel = decel;
 
     } else if (strcmp(name, "cis") == 0) {
         long cur_pos = atol(value);
 
         params->current_position = cur_pos;
 
     } else if (strcmp(name, "fis") == 0) {
         unsigned long fin_pos = atol(value);
         params->final_position = fin_pos;
 
     } else if (strcmp(name, "sm") == 0) {
         int smode = atoi(value);
         if (smode < 0 || smode > 2) {
             xil_printf("Invalid step mode: %d. Defaulting to 0 (full step).\r\n", smode);
             smode = 0;
         }
         params->step_mode = smode;
 
     } else if (strcmp(name, "dt") == 0) {
         int dt = atoi(value);
         if (dt < 0) {
             xil_printf("Invalid dwell time: %d. Setting to 0.\r\n", dt);
             dt = 0;
         }
         params->dwell_time = dt;
 
 
     } else {
         xil_printf("Unrecognized parameter: %s\r\n", name);
         recognized = 0;
     }
 
     return recognized;
 }