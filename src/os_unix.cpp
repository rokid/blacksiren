/*
 * OS specific functions for UNIX/POSIX systems
 * Copyright (c) 2005-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef ANDROID
#include <sys/capability.h>
#include <sys/prctl.h>
#include <private/android_filesystem_config.h>
#endif /* ANDROID */

#include "../include/os.h"
#include "../include/common.h"

void os_sleep(os_time_t sec, os_time_t usec)
{
    if (sec)
        sleep(sec);
    if (usec)
        usleep(usec);
}

int os_get_time(struct os_time* t)
{
    int res;
    struct timeval tv;
    res = gettimeofday(&tv, NULL);
    t->sec = tv.tv_sec;
    t->usec = tv.tv_usec;
    return res;
}

#define CLOCK_MONOTONIC 1
int os_get_reltime(struct os_reltime* t)
{
#if defined(CLOCK_BOOTTIME)
    static clockid_t clock_id = CLOCK_BOOTTIME;
#elif defined(CLOCK_MONOTONIC)
    static clockid_t clock_id = CLOCK_MONOTONIC;
#else
    static clockid_t clock_id = CLOCK_REALTIME;
#endif
    struct timespec ts;
    int res;

    while (1) {
        res = clock_gettime(clock_id, &ts);
        if (res == 0) {
            t->sec = ts.tv_sec;
            t->usec = ts.tv_nsec / 1000;
            return 0;
        }
        switch (clock_id) {
#ifdef CLOCK_BOOTTIME
        case CLOCK_BOOTTIME:
            clock_id = CLOCK_MONOTONIC;
            break;
#endif
#ifdef CLOCK_MONOTONIC
        case CLOCK_MONOTONIC:
            clock_id = CLOCK_REALTIME;
            break;
#endif
        case CLOCK_REALTIME:
            return -1;
        }
    }
}

int os_mktime(
    int year, int month, int day, int hour, int min, int sec, os_time_t* t)
{
    struct tm tm, *tm1;
    time_t t_local, t1, t2;
    os_time_t tz_offset;

    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31
        || hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 60)
        return -1;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    t_local = mktime(&tm);

    /* figure out offset to UTC */
    tm1 = localtime(&t_local);
    if (tm1) {
        t1 = mktime(tm1);
        tm1 = gmtime(&t_local);
        if (tm1) {
            t2 = mktime(tm1);
            tz_offset = t2 - t1;
        } else
            tz_offset = 0;
    } else
        tz_offset = 0;

    *t = (os_time_t)t_local - tz_offset;
    return 0;
}

int os_gmtime(os_time_t t, struct os_tm* tm)
{
    struct tm* tm2;
    time_t t2 = t;

    tm2 = gmtime(&t2);
    if (tm2 == NULL)
        return -1;
    tm->sec = tm2->tm_sec;
    tm->min = tm2->tm_min;
    tm->hour = tm2->tm_hour;
    tm->day = tm2->tm_mday;
    tm->month = tm2->tm_mon + 1;
    tm->year = tm2->tm_year + 1900;
    return 0;
}

int os_get_random(unsigned char* buf, size_t len)
{
    FILE* f;
    size_t rc;

    f = fopen("/dev/urandom", "rb");
    if (f == NULL) {
        printf("Could not open /dev/urandom.\n");
        return -1;
    }

    rc = fread(buf, 1, len, f);
    fclose(f);

    return rc != len ? -1 : 0;
}

unsigned long os_random(void) { return random(); }

int os_setenv(const char* name, const char* value, int overwrite)
{
    return setenv(name, value, overwrite);
}

int os_unsetenv(const char* name) { return unsetenv(name); }

char* os_readfile(const char* name, size_t* len)
{
    FILE* f;
    char* buf;
    long pos;

    f = fopen(name, "rb");
    if (f == NULL)
        return NULL;

    if (fseek(f, 0, SEEK_END) < 0 || (pos = ftell(f)) < 0) {
        fclose(f);
        return NULL;
    }
    *len = pos;
    if (fseek(f, 0, SEEK_SET) < 0) {
        fclose(f);
        return NULL;
    }

    buf = static_cast<char *>(malloc(*len));
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }

    if (fread(buf, 1, *len, f) != *len) {
        fclose(f);
        free(buf);
        return NULL;
    }

    fclose(f);

    return buf;
}

int os_file_exists(const char* fname)
{
    FILE* f = fopen(fname, "rb");
    if (f == NULL)
        return 0;
    fclose(f);
    return 1;
}

int os_fsync(FILE* stream)
{
    if (!fflush(stream))
        return fsync(fileno(stream));
    return -1;
}

int os_exec(const char* program, const char* arg, int wait_completion)
{
    pid_t pid;
    int pid_status;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* run the external command in the child process */
        const int MAX_ARG = 30;
        char *_program, *_arg, *pos;
        char* argv[MAX_ARG + 1];
        int i;

        _program = strdup(program);
        _arg = strdup(arg);

        argv[0] = _program;

        i = 1;
        pos = _arg;
        while (i < MAX_ARG && pos && *pos) {
            while (*pos == ' ')
                pos++;
            if (*pos == '\0')
                break;
            argv[i++] = pos;
            pos = strchr(pos, ' ');
            if (pos)
                *pos++ = '\0';
        }
        argv[i] = NULL;

        execv(program, argv);
        perror("execv");
        free(_program);
        free(_arg);
        exit(0);
        return -1;
    }

    if (wait_completion) {
        /* wait for the child process to complete in the parent */
        waitpid(pid, &pid_status, 0);
    }

    return 0;
}
