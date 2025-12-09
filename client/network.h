#ifndef NETWORK_H
#define NETWORK_H

int connect_to_server(const char *host, int port);
void disconnect_from_server(int socket);
int send_request(int socket, const char *request);
int receive_response(int socket, char *buffer, int buffer_size);

#endif