// Copyright SCI Semiconductor and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#pragma once
#include <compartment-macros.h>
#include <errno.h>
#include <futex.h>
#include <riscvreg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <timeout.h>

typedef int64_t  time_t;      // NOLINT
typedef uint32_t suseconds_t; // NOLINT

struct timeval // NOLINT
{
	time_t      tv_sec;  // NOLINT
	suseconds_t tv_usec; // NOLINT
};

/**
 * The pair of a time synchronised from NTP and the cycle count where this time
 * was read.
 */
struct SynchronisedTime
{
	_Atomic(uint32_t) updatingEpoch;
	struct timeval    time;
	uint64_t          cycles;
};

/**
 * Update the time using SNTP.  This updates the value stored in the
 * `SynchronisedTime` structure returned by `sntp_time_get()`.
 */
int __cheri_compartment("SNTP") sntp_update(Timeout *timeout);

/**
 * Returns a read-only pointer to the synchronised time structure.  This can be
 * used to get the current time (modulo clock drift) without a
 * cross-compartment call.
 */
struct SynchronisedTime *__cheri_compartment("SNTP") sntp_time_get(void);

/**
 * Library call to compute a timeval from the previous timeval synchronised
 * with NTP.  The first argument is storage for a cached pointer retrieved from
 * the SNTP compartment.  This can be an on-stack ephemeral value if caching is
 * not desired, but must be initialised to either NULL or the return value from
 * `sntp_time_get`.
 */
int __cheri_libcall timeval_calculate(struct timeval *__restrict tp,
                                      struct SynchronisedTime **sntpTimeCache);

/**
 * POSIX-compatible gettimeofday() implementation that uses the SNTP time.
 *
 * Calculates the time based on the number of cycles that have elapsed since
 * the last update from SNTP.
 */
static inline int gettimeofday(struct timeval *__restrict tp,
                               void *__restrict tzp)
{
	(void)tzp;
	static struct SynchronisedTime *sntpTime = NULL;
	return timeval_calculate(tp, &sntpTime);
}

/**
 * POSIX-compatible time() implementation that uses the SNTP time.
 *
 * Calculates the time based on the number of cycles that have elapsed since
 * the last update from SNTP.
 */
static inline time_t time(time_t *tloc)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1)
	{
		// C-style cast required because this file can be included in C.
		return (time_t)-1; // NOLINT
	}
	if (tloc != NULL)
	{
		*tloc = tv.tv_sec;
	}
	return tv.tv_sec;
}
