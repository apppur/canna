#ifndef _CANNA_DAEMON_H
#define _CANNA_DAEMON_H

#define SERVER_INIT_DAEMON 0

int daemon_init(int mode, const char *pidfile);
int daemon_exit(const char *pidfile);

#endif
