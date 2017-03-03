/*
 * OS specific functions
 * Copyright (c) 2005-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef OS_H
#define OS_H

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#ifdef ANDROID
#include <android/log.h>
#endif

__BEGIN_DECLS

typedef long os_time_t;

/**
 * os_sleep - Sleep (sec, usec)
 * @sec: Number of seconds to sleep
 * @usec: Number of microseconds to sleep
 */
void os_sleep(os_time_t sec, os_time_t usec);

struct os_time {
	os_time_t sec;
	os_time_t usec;
};

struct os_reltime {
	os_time_t sec;
	os_time_t usec;
};

/**
 * os_get_time - Get current time (sec, usec)
 * @t: Pointer to buffer for the time
 * Returns: 0 on success, -1 on failure
 */
int os_get_time(struct os_time *t);

/**
 * os_get_reltime - Get relative time (sec, usec)
 * @t: Pointer to buffer for the time
 * Returns: 0 on success, -1 on failure
 */
int os_get_reltime(struct os_reltime *t);


/* Helpers for handling struct os_time */

static inline int os_time_before(struct os_time *a, struct os_time *b)
{
	return (a->sec < b->sec) ||
	       (a->sec == b->sec && a->usec < b->usec);
}


static inline void os_time_sub(struct os_time *a, struct os_time *b,
			       struct os_time *res)
{
	res->sec = a->sec - b->sec;
	res->usec = a->usec - b->usec;
	if (res->usec < 0) {
		res->sec--;
		res->usec += 1000000;
	}
}


/* Helpers for handling struct os_reltime */

static inline int os_reltime_before(struct os_reltime *a,
				    struct os_reltime *b)
{
	return (a->sec < b->sec) ||
	       (a->sec == b->sec && a->usec < b->usec);
}


static inline void os_reltime_sub(struct os_reltime *a, struct os_reltime *b,
				  struct os_reltime *res)
{
	res->sec = a->sec - b->sec;
	res->usec = a->usec - b->usec;
	if (res->usec < 0) {
		res->sec--;
		res->usec += 1000000;
	}
}


static inline void os_reltime_age(struct os_reltime *start,
				  struct os_reltime *age)
{
	struct os_reltime now;

	os_get_reltime(&now);
	os_reltime_sub(&now, start, age);
}


static inline int os_reltime_expired(struct os_reltime *now,
				     struct os_reltime *ts,
				     os_time_t timeout_secs)
{
	struct os_reltime age;

	os_reltime_sub(now, ts, &age);
	return (age.sec > timeout_secs) ||
	       (age.sec == timeout_secs && age.usec > 0);
}


static inline int os_reltime_initialized(struct os_reltime *t)
{
	return t->sec != 0 || t->usec != 0;
}


/**
 * os_mktime - Convert broken-down time into seconds since 1970-01-01
 * @year: Four digit year
 * @month: Month (1 .. 12)
 * @day: Day of month (1 .. 31)
 * @hour: Hour (0 .. 23)
 * @min: Minute (0 .. 59)
 * @sec: Second (0 .. 60)
 * @t: Buffer for returning calendar time representation (seconds since
 * 1970-01-01 00:00:00)
 * Returns: 0 on success, -1 on failure
 *
 * Note: The result is in seconds from Epoch, i.e., in UTC, not in local time
 * which is used by POSIX mktime().
 */
int os_mktime(int year, int month, int day, int hour, int min, int sec,
	      os_time_t *t);

struct os_tm {
	int sec; /* 0..59 or 60 for leap seconds */
	int min; /* 0..59 */
	int hour; /* 0..23 */
	int day; /* 1..31 */
	int month; /* 1..12 */
	int year; /* Four digit year */
};

int os_gmtime(os_time_t t, struct os_tm *tm);

/**
 * os_get_random - Get cryptographically strong pseudo random data
 * @buf: Buffer for pseudo random data
 * @len: Length of the buffer
 * Returns: 0 on success, -1 on failure
 */
int os_get_random(unsigned char *buf, size_t len);

/**
 * os_random - Get pseudo random value (not necessarily very strong)
 * Returns: Pseudo random value
 */
unsigned long os_random(void);

/**
 * os_setenv - Set environment variable
 * @name: Name of the variable
 * @value: Value to set to the variable
 * @overwrite: Whether existing variable should be overwritten
 * Returns: 0 on success, -1 on error
 *
 * This function is only used for wpa_cli action scripts. OS wrapper does not
 * need to implement this if such functionality is not needed.
 */
int os_setenv(const char *name, const char *value, int overwrite);

/**
 * os_unsetenv - Delete environent variable
 * @name: Name of the variable
 * Returns: 0 on success, -1 on error
 *
 * This function is only used for wpa_cli action scripts. OS wrapper does not
 * need to implement this if such functionality is not needed.
 */
int os_unsetenv(const char *name);

/**
 * os_readfile - Read a file to an allocated memory buffer
 * @name: Name of the file to read
 * @len: For returning the length of the allocated buffer
 * Returns: Pointer to the allocated buffer or %NULL on failure
 *
 * This function allocates memory and reads the given file to this buffer. Both
 * binary and text files can be read with this function. The caller is
 * responsible for freeing the returned buffer with os_free().
 */
char * os_readfile(const char *name, size_t *len);

/**
 * os_file_exists - Check whether the specified file exists
 * @fname: Path and name of the file
 * Returns: 1 if the file exists or 0 if not
 */
int os_file_exists(const char *fname);

/**
 * os_fsync - Sync a file's (for a given stream) state with storage device
 * @stream: the stream to be flushed
 * Returns: 0 if the operation succeeded or -1 on failure
 */
int os_fsync(FILE *stream);

__END_DECLS

#endif /* OS_H */
