#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

void* notification_thread(void *arg);
void start_notification_listener(int socket);
void stop_notification_listener();

#endif