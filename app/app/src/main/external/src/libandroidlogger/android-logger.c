/*
 * android-logger.c
 *
 *  Created on: Jun 1, 2016
 *      Author: fabian
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include "android-logger.h"

static int pfd[2] = { -1, -1 };
static pthread_t thr;
static const char *tag = "myapp";
static int logger_started = 0;

static void *thread_func(void *arg)
{
    ssize_t rdsz;
    char buf[128];

    (void)arg;
    while ((rdsz = read(pfd[0], buf, sizeof(buf) - 1)) > 0) {
        if (rdsz > 0 && buf[rdsz - 1] == '\n')
            --rdsz;
        buf[rdsz] = '\0';  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    return NULL;
}

int start_logger(const char *app_name)
{
    if (app_name != NULL)
        tag = app_name;

    if (logger_started)
        return 0;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    if (pipe(pfd) < 0)
        return -1;

    if (dup2(pfd[1], STDOUT_FILENO) < 0 || dup2(pfd[1], STDERR_FILENO) < 0) {
        close(pfd[0]);
        close(pfd[1]);
        pfd[0] = pfd[1] = -1;
        return -1;
    }

    if (pfd[1] != STDOUT_FILENO && pfd[1] != STDERR_FILENO) {
        close(pfd[1]);
        pfd[1] = -1;
    }

    /* spawn the logging thread */
    if (pthread_create(&thr, NULL, thread_func, NULL) != 0) {
        close(pfd[0]);
        pfd[0] = -1;
        return -1;
    }
    logger_started = 1;
    pthread_detach(thr);
    return 0;
}



