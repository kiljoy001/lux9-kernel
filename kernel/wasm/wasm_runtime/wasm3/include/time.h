#ifndef _TIME_H_
#define _TIME_H_

#include <u.h>

typedef long time_t;
typedef long clock_t;

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

/* Kernel time functions */
/* We can just stub these or map to kernel time if needed */
/* For now, just declarations to satisfy compiler */

time_t time(time_t *t);
clock_t clock(void);

#endif
