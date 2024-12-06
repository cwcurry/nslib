/* cpl_timedate.c -- Functions using time and date.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "config.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "cpl_timedate.h"
#include "cpl_str.h"


/* Nanoseconds Per Second */
#define NSEC_PER_SEC 1000000000


/******************************************************************************
*
* cpl_is_leap_year - Return non-zero if YEAR is a leap year.
*
******************************************************************************/

int cpl_is_leap_year (
    const int year) {

    return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}


/******************************************************************************
*
* cpl_ordinal_day - Return the ordinal day or day-of-the-year for the given
*   YEAR, MONTH, and DAY.  The caller should make sure that the month is in
*   the valid range.
*
******************************************************************************/

int cpl_ordinal_day (
    const int year,
    const int month,
    const int day) {

    static const int cdays[] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    int ordinal_day;


    assert ((month >= 1) && (month <= 12));

    ordinal_day = cdays[month] + day;

    if ((month > 2) && (cpl_is_leap_year (year))) {
        ordinal_day += 1;
    }

    return ordinal_day;
}


/******************************************************************************
*
* cpl_month_day_from_ordinal_day - Calculate the month and day number from the
*   ORDINAL_DAY and YEAR and return in MONTH and MDAY.  The caller should make
*   sure the ordinal day is in the range [1, 366].
*
******************************************************************************/

void cpl_month_day_from_ordinal_day (
    int *month,
    int *mday,
    const int year,
    const int ordinal_day) {

    int j;


    assert (month);
    assert (mday);
    assert (ordinal_day >= 1);
    assert (ordinal_day <= 366);

    /* TODO: Convert this to LUT? */
    j = (ordinal_day + 305);

    if (cpl_is_leap_year (year)) {
        j = j % 366;
    } else {
        j = j % 365;
    }

    j = (j % 153) / 61 + 2 * (j / 153) + j;

    *month = (j / 31 + 2) % 12 + 1;
    *mday = j % 31 + 1;
}


/******************************************************************************
*
* cpl_days_in_month - Return the number of days in the MONTH given the YEAR.
*   The caller should make sure that the month is in the valid range.
*
******************************************************************************/

int cpl_days_in_month (
    const int year,
    const int month) {

    int days;


    assert ((month >= 1) && (month <= 12));

    if ((month == 2) && (cpl_is_leap_year (year))) {
        days = 29;
    } else {
        static const int mdays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        days = mdays[month];
    }

    return days;
}


/******************************************************************************
*
* cpl_valid_date - Return non-zero if the date/time inputs are in a valid range.
*
******************************************************************************/

int cpl_valid_date (
    const int year,
    const int month,
    const int day,
    const int hour,
    const int minute,
    const double second) {

    if ((month <= 0) || (month > 12)) return 0;
    if ((day <= 0) || (day > cpl_days_in_month (year, month))) return 0;
    if ((hour < 0) || (hour > 23)) return 0;
    if ((minute < 0) || (minute > 59)) return 0;
    if ((second < 0.0) || (second > 60.0)) return 0;  /* Allow leap second (second == 60). */

    return 1;
}


/******************************************************************************
*
* cpl_strftime - Print a formatted character string of a broken-down UTC
*   time from a time_spec structure, T, to a user-defined buffer S of length
*   MAX_SIZE.  The format of the string is given by several pre-defined FORMAT
*   types.  The following formats are specified:
*
*   1 - 'Tue Jan 31 02:25:50 UTC 2012'.
*   2 - '2012:01:31 02:25:50'.
*   3 - '2012-01-31T02:25:50Z'.     Used by Google Earth KML Format.
*   4 - '2012/01/31 02:25:50.50000
*
*   Additional formats can be added as needed.
*
* Return: The number of characters that would have been written if MAX_SIZE had
*   been sufficiently large, not counting the terminating null character.  If
*   an encoding error occurs, then a negative number is returned.
*
******************************************************************************/

int cpl_strftime (
    char *s,
    const size_t max_size,
    const struct timespec *t,
    const int format) {

    static const char * const afmt[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char * const bfmt[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    struct tm tp;
    double fsecs;


    assert (s);
    assert (t);

    if (max_size < 1) return 0;

    /* Convert time_t to a broken-down time structure. */
    if (cpl_gmtime (&(t->tv_sec), &tp) != 0) return 0;

    switch (format) {

        case CPL_STRFTIME_FMT_1 :
            if ((tp.tm_wday < 0) || (tp.tm_wday > 6)) return -1;
            if ((tp.tm_mon < 0) || (tp.tm_mon > 11)) return -1;
            return snprintf (s, max_size, "%s %s %d %02d:%02d:%02d UTC %04d", afmt[tp.tm_wday], bfmt[tp.tm_mon], tp.tm_mday, tp.tm_hour, tp.tm_min, tp.tm_sec, 1900 + tp.tm_year);
            break;

        case CPL_STRFTIME_FMT_2 :
            return snprintf (s, max_size, "%04d:%02d:%02d %02d:%02d:%02d", 1900 + tp.tm_year, tp.tm_mon + 1, tp.tm_mday, tp.tm_hour, tp.tm_min, tp.tm_sec);
            break;

        case CPL_STRFTIME_FMT_3 :
            return snprintf (s, max_size, "%04d-%02d-%02dT%02d:%02d:%02dZ", 1900 + tp.tm_year, tp.tm_mon + 1, tp.tm_mday, tp.tm_hour, tp.tm_min, tp.tm_sec);
            break;

        case CPL_STRFTIME_FMT_4 :
            fsecs = 1.0e-9 * t->tv_nsec;
            return snprintf (s, max_size, "%02d/%02d/%02d %02d:%02d:%08.5f", 1900 + tp.tm_year, tp.tm_mon + 1, tp.tm_mday, tp.tm_hour, tp.tm_min, tp.tm_sec + fsecs);
            break;

        default: assert (0); break;
    }

    return 0;
}


/******************************************************************************
*
* cpl_gmtime - Convert a time_t value T to a broken-down UTC time in TP.
*
* Return: 0 if the conversion was successful, and
*         non-zero if an error occurred.
*
******************************************************************************/

int cpl_gmtime (
    const time_t *t,
    struct tm *tp) {

#if defined CPL_WIN32_API

    assert (t);
    assert (tp);

    if (gmtime_s (tp, t) != 0) {
        return -1;
    }

#elif defined HAVE_GMTIME_R

    struct tm *ret;


    assert (t);
    assert (tp);

    memset (tp, 0, sizeof (*tp));

    ret = gmtime_r (t, tp);
    if (!ret) return -1;

    /* Some implementations (FreeBSD) don't return NULL on overflow.  So we
       check mday here since it can't otherwise be zero. */
    if (ret->tm_mday == 0) {
        return -1;
    }

#else

    struct tm *ret;


    assert (t);
    assert (tp);

    ret = gmtime (t);
    if (!ret) return -1;

    if (ret->tm_mday == 0) {
        return -1;
    }

    *tp = *ret;
#endif

    return 0;
}


/******************************************************************************
*
* cpl_mktime - Convert (Gregorian) date to UTC seconds since the UNIX Epoch
*   (1970-01-01 00:00:00).  Unlike mktime no timezone change is made to local time.
*
* Input: year - Year in the range [1970-2105],
*        month - Month in the range [1-12],
*        day - Day of the month in the range [1-31],
*        hour - Hour in the range [0-23],
*        minute - Minute in the range [0-59],
*        second - Second in the range [0-60].
*
* Return: Time in seconds since the UNIX Epoch, or
*        -1 if the input date is invalid.
*
******************************************************************************/

double cpl_mktime (
    const int year,
    const int month,
    const int day,
    const int hour,
    const int minute,
    const double second) {

    unsigned long days;
    int m = month;
    int y = year;


    /* Sanity check input. */
    if (year < 1970) return -1.0;
    if (cpl_valid_date (year, month, day, hour, minute, second) == 0) return -1.0;

    /* Move January and February to the previous year. */
    m = m - 2;

    if (m <= 0) {
        m = m + 12;
        y = y - 1;
    }

    /* Calculate the number of days since 1970 including leap days. */
    days = y / 4 - y / 100 + y / 400 + 367 * m / 12 + day + 365 * y - 719499;

    /* Convert to seconds. */
    return ((days * 24.0 + hour) * 60.0 + minute) * 60.0 + second;
}


/******************************************************************************
*
* cpl_jd_from_utc_time - Convert UTC time to Julian Date.
*
******************************************************************************/

double cpl_jd_from_utc_time (
    const double utc_time) {

    return (utc_time / 86400.0) + 2440587.5;
}


/******************************************************************************
*
* cpl_jd_from_utc_date - Convert (Gregorian) date to Julian date, which is the
*   number of days since the beginning of the Julian Period (noon on Jan 1,
*   4713 BCE).
*
* Input: year - Year,
*        mon - Month in the range [1 - 12],
*        day - Day of the month in the range [1 - 31],
*        hour - Hour in the range [0-23],
*        min - Minute in the range [0-59],
*        sec - Second in the range [0-60].
*
* Return: Days since the Julian Period, or
*        -1 if the input date is invalid.
*
******************************************************************************/

double cpl_jd_from_utc_date (
    const int year,
    const int month,
    const int day,
    const int hour,
    const int minute,
    const double second) {

    double julian_date;
    int m, y;


    /* Sanity check input. */
    if (cpl_valid_date (year, month, day, hour, minute, second) == 0) return -1.0;

    if (month <= 2) {
        m = month + 12;
        y = year - 1;
    } else {
        y = year;
        m = month;
    }

    julian_date = trunc (365.25 * y) + trunc (30.6001 * (m + 1)) + day + hour / 24.0 + minute / 1440.0 + second / 86400.0 + 1720981.5;

    return julian_date;
}


/******************************************************************************
*
* cpl_utc_time_from_jd - Convert Julian Date JD to UTC time.
*
******************************************************************************/

double cpl_utc_time_from_jd (
    const double jd) {

    return (jd - 2440587.5) * 86400.0;
}


/******************************************************************************
*
* cpl_utc_date_from_jd - Convert JULIAN_DATE to UTC time.
*
******************************************************************************/

void cpl_utc_date_from_jd (
    int *utc_year,
    int *utc_month,
    int *utc_day,
    int *utc_hour,
    int *utc_minute,
    double *utc_seconds,
    const double julian_date) {

    int a, b, c, d, e;
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    double seconds;
    double td;


    assert (julian_date >= 0.0);

    a = (int) (julian_date + 0.5);
    b = a + 1537;
    c = (int) (((double) b - 122.1) / 365.25);
    d = (int) (365.25 * c);
    e = (int) (((double) (b - d)) / 30.6001);

    td = b - d - (int) (30.6001 * e) + fmod (julian_date + 0.5, 1.0);
    day = (unsigned int) td;
    td -= day;
    td *= 24.0;
    hour = (unsigned int) td;
    td -= hour;
    td *= 60.0;
    minute = (unsigned int) td;
    td -= minute;
    td *= 60.0;
    seconds = td;
    month = (unsigned int) (e - 1 - 12 * (e / 14));
    year = (unsigned int) (c - 4715 - (int) ((7.0 + (double) month) / 10.0));

    if (seconds >= 60.0) {
        seconds -= 60.0;
        minute++;
        if (minute >= 60) {
            minute -= 60;
            hour++;
            if (hour >= 24) {
                unsigned int days_in_month;
                hour -= 24;
                day++;

                days_in_month = cpl_days_in_month (year, month);

                if (day > days_in_month) {
                    day = 1;
                    month++;
                    if (month > 12) {
                        month = 1;
                        year++;
                    }
                }
            }
        }
    }

    *utc_year = year;
    *utc_month = month;
    *utc_day = day;
    *utc_hour = hour;
    *utc_minute = minute;
    *utc_seconds = seconds;
}


/******************************************************************************
*
* cpl_utc_offset - Return the number of seconds that GPS is ahead of UTC time
*   (always positive) given the JULIAN_DATE (number of days since noon Universal
*   Time Jan 1, 4713 BCE).
*
******************************************************************************/

int cpl_utc_offset (
    const double julian_date) {

    assert (julian_date >= 0.0);

    if (julian_date < 2444786.5) return 0;   /* 1981 JUL 1 TAI-UTC=20 */
    if (julian_date < 2445151.5) return 1;   /* 1982 JUL 1 TAI-UTC=21 */
    if (julian_date < 2445516.5) return 2;   /* 1983 JUL 1 TAI-UTC=22 */
    if (julian_date < 2446247.5) return 3;   /* 1985 JUL 1 TAI-UTC=23 */
    if (julian_date < 2447161.5) return 4;   /* 1988 JAN 1 TAI-UTC=24 */
    if (julian_date < 2447892.5) return 5;   /* 1990 JAN 1 TAI-UTC=25 */
    if (julian_date < 2448257.5) return 6;   /* 1991 JAN 1 TAI-UTC=26 */
    if (julian_date < 2448804.5) return 7;   /* 1992 JUL 1 TAI-UTC=27 */
    if (julian_date < 2449169.5) return 8;   /* 1993 JUL 1 TAI-UTC=28 */
    if (julian_date < 2449534.5) return 9;   /* 1994 JUL 1 TAI-UTC=29 */
    if (julian_date < 2450083.5) return 10;  /* 1996 JAN 1 TAI-UTC=30 */
    if (julian_date < 2450630.5) return 11;  /* 1997 JUL 1 TAI-UTC=31 */
    if (julian_date < 2451179.5) return 12;  /* 1999 JAN 1 TAI-UTC=32 */
    if (julian_date < 2453736.5) return 13;  /* 2006 JAN 1 TAI-UTC=33 */
    if (julian_date < 2454832.5) return 14;  /* 2009 JAN 1 TAI-UTC=34 */
    if (julian_date < 2456109.5) return 15;  /* 2012 JUL 1 TAI-UTC=35 */
    if (julian_date < 2457204.5) return 16;  /* 2015 JUL 1 TAI-UTC=36 */
    if (julian_date < 2457754.5) return 17;  /* 2017 JAN 1 TAI-UTC=37 */

    return 18;
}


/******************************************************************************
*
* cpl_jd_from_gps_time - Convert the GPS_WEEK (0-1024) and GPS time of
*   week, GPS_TOW, and UTC_OFFSET to Julian Date.
*
******************************************************************************/

double cpl_jd_from_gps_time (
    const unsigned int gps_week,
    const double gps_tow,
    const unsigned int utc_offset) {

    assert (gps_tow >= 0.0);
    assert (gps_tow <= 604800.0);

    return (gps_week + (gps_tow - utc_offset) / 604800.0) * 7.0 + 2444244.5;
}


/******************************************************************************
*
* cpl_timespec_to_double - Convert time given by struct timespec object TS to
*   a double and return it.
*
******************************************************************************/

double cpl_timespec_to_double (
    const struct timespec *ts) {

    assert (ts);
    return ts->tv_sec + 1.0e-9 * ts->tv_nsec;
}


/******************************************************************************
*
* cpl_timespec_from_double - Convert a time stored as seconds in a double type T
*   to a struct timespec type stored in TS.
*
******************************************************************************/

void cpl_timespec_from_double (
    struct timespec *ts,
    const double t) {

    double x = t + 0.5e-9;


    assert (ts);

    ts->tv_sec = (long) x;
    ts->tv_nsec = (x - ts->tv_sec) * NSEC_PER_SEC;
    cpl_timespec_normalize (ts);
}


/******************************************************************************
*
* cpl_timespec_add - Add two timespec values, TS1 = TS1 + TS2.
*
******************************************************************************/

void cpl_timespec_add (
    struct timespec *ts1,
    struct timespec *ts2) {

    assert (ts1);
    assert (ts2);

    cpl_timespec_normalize (ts1);
    cpl_timespec_normalize (ts2);

    ts1->tv_sec += ts2->tv_sec;
    ts1->tv_nsec += ts2->tv_nsec;

    cpl_timespec_normalize (ts1);
}


/******************************************************************************
*
* cpl_timespec_sub - Subtract two timespec values, TS1 = TS1 - TS2.
*
******************************************************************************/

void cpl_timespec_sub (
    struct timespec *ts1,
    struct timespec *ts2) {

    assert (ts1);
    assert (ts2);

    cpl_timespec_normalize (ts1);
    cpl_timespec_normalize (ts2);

    ts1->tv_sec -= ts2->tv_sec;
    ts1->tv_nsec -= ts2->tv_nsec;

    cpl_timespec_normalize (ts1);
}


/******************************************************************************
*
* cpl_timespec_cmp - Return -1 if the timespec value TS1 is less than TS2;
*   1 if the timespec value TS1 is greater than TS2; and zero if equal.
*
******************************************************************************/

int cpl_timespec_cmp (
    struct timespec *ts1,
    struct timespec *ts2) {

    assert (ts1);
    assert (ts2);

    cpl_timespec_normalize (ts1);
    cpl_timespec_normalize (ts2);

    if (ts1->tv_sec < ts2->tv_sec) return -1;
    if (ts1->tv_sec > ts2->tv_sec) return  1;
    if (ts1->tv_nsec < ts2->tv_nsec) return -1;
    if (ts1->tv_nsec > ts2->tv_nsec) return  1;
    return 0;
}


/******************************************************************************
*
* cpl_timespec_normalize - Normalize the timespec value TS.  This puts tv_nsec
*   in the range [0, NSEC_PER_SEC-1].
*
******************************************************************************/

void cpl_timespec_normalize (
    struct timespec *ts) {

    assert (ts);

    if ((ts->tv_nsec < 0) || (ts->tv_nsec >= NSEC_PER_SEC)) {

        ts->tv_sec += ts->tv_nsec / NSEC_PER_SEC;
        ts->tv_nsec = ts->tv_nsec % NSEC_PER_SEC;

        if (ts->tv_nsec < 0) {
            ts->tv_sec--;
            ts->tv_nsec += NSEC_PER_SEC;
        }
    }
}
