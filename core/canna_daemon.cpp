#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "canna_daemon.h"

#define SERVER_INIT_DAEMON 0
#define COMMAND_LEN 256

int daemon_init(int mode, const char *pidfile)
{
    if (SERVER_INIT_DAEMON == mode)
    {
        return 0;
    }

    pid_t pid;
    if ((pid = fork()) != 0)
    {
        exit(0);
    }

    setsid();

    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGHUP, &sig, NULL);

    if ((pid == fork()) != 0)
    {
        exit(0);
    }

    int pid_fd = open(pidfile, O_RDWR|O_CREAT, 0644);
    if (pid_fd < 0)
    {
        printf("open pid file failed, daemon init failed\n");
        return -1;
    }
    FILE* pfile = fdopen(pid_fd, "w+");
    if (pfile == NULL)
    {
        printf("fdopen pid file failed, daemon init failed\n");
        return -2;
    }
    if (flock(pid_fd, LOCK_EX|LOCK_NB) == -1)
    {
        printf("lock pid file failed, daemon is already running\n");
        return -3;
    }
    pid = getpid();
    if (!fprintf(pfile, "%d\n", pid))
    {
        printf("write pid to file failed\n");
        close(pid_fd);
        return -4;
    }
    fflush(pfile);

    umask(0);
    setpgrp();

    return 0;
}

int daemon_exit(const char *pidfile)
{
    return unlink(pidfile);
}

