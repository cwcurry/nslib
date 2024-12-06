/* cpl_timedate.h -- Header file for cpl_timedate.c

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

#ifndef CPL_TIMEDATE_H
#define CPL_TIMEDATE_H

#if defined (__cplusplus)
#include <ctime>
#else
#include <time.h>
#endif

#include "cpl_spec.h"


/* cpl_strftime Format Definitions */
#define CPL_STRFTIME_FMT_1  1
#define CPL_STRFTIME_FMT_2  2
#define CPL_STRFTIME_FMT_3  3
#define CPL_STRFTIME_FMT_4  4


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

int cpl_is_leap_year (const int) CPL_ATTRIBUTE_CONST;
int cpl_ordinal_day (const int, const int, const int) CPL_ATTRIBUTE_CONST;
void cpl_month_day_from_ordinal_day (int *, int *, const int, const int) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_days_in_month (const int, const int) CPL_ATTRIBUTE_CONST;
int cpl_valid_date (const int, const int, const int, const int, const int, const double) CPL_ATTRIBUTE_CONST;
int cpl_strftime (char *, const size_t, const struct timespec *, const int) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_gmtime (const time_t *, struct tm *) CPL_ATTRIBUTE_NONNULL_ALL;
double cpl_mktime (const int, const int, const int, const int, const int, const double) CPL_ATTRIBUTE_CONST;
double cpl_jd_from_utc_time (const double) CPL_ATTRIBUTE_CONST;
double cpl_jd_from_utc_date (const int, const int, const int, const int, const int, const double) CPL_ATTRIBUTE_CONST;
double cpl_utc_time_from_jd (const double) CPL_ATTRIBUTE_CONST;
void cpl_utc_date_from_jd (int *, int *, int *, int *, int *, double *, const double) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_utc_offset (const double) CPL_ATTRIBUTE_CONST;
double cpl_jd_from_gps_time (const unsigned int, const double, const unsigned int) CPL_ATTRIBUTE_CONST;
double cpl_timespec_to_double (const struct timespec *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
void cpl_timespec_from_double (struct timespec *, const double) CPL_ATTRIBUTE_NONNULL_ALL;
void cpl_timespec_add (struct timespec *, struct timespec *) CPL_ATTRIBUTE_NONNULL_ALL;
void cpl_timespec_sub (struct timespec *, struct timespec *) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_timespec_cmp (struct timespec *, struct timespec *) CPL_ATTRIBUTE_NONNULL_ALL;
void cpl_timespec_normalize (struct timespec *) CPL_ATTRIBUTE_NONNULL_ALL;


CPL_CLINKAGE_END

#endif /* CPL_TIMEDATE_H */
