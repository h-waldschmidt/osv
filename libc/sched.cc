/*
 * Copyright (C) 2014 Huawei Technologies Duesseldorf GmbH
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#include <errno.h>
#include <api/sched.h>
#include <api/sys/resource.h>
#include <osv/sched.hh>

#include <osv/stubbing.hh>

#define SCHED_PRIO_MIN 1
#define SCHED_PRIO_MAX 99

/* sched_rr_get_interval writes the time quantum for SCHED_RR
 * processes to the tp timespec structure.
 * Since the SCHED_RR is currently not implemented, we don't
 * implement this function yet.
 */

int sched_rr_get_interval(pid_t pid, struct timespec * tp)
{
 WARN_STUBBED();
 errno = ENOSYS;
 return -1;
}

static int sched_get_priority_minmax(int policy, int value)
{
 switch (policy) {
 case SCHED_OTHER:
 return 0;
 case SCHED_FIFO:
 return value;
 default:
 errno = EINVAL;
 /* error return value unfortunately overlaps with allowed
 * POSIX API allowed range. Another good reason to have
 * only rt priorities > 0.
 */
 return -1;
 }
}

int sched_get_priority_min(int policy)
{
 return sched_get_priority_minmax(policy, SCHED_PRIO_MIN);
}

int sched_get_priority_max(int policy)
{
 return sched_get_priority_minmax(policy, SCHED_PRIO_MAX);
}

static int sched_setparam_aux(sched::thread *t, int policy, int prio)
{
 switch (policy) {
 case SCHED_OTHER:
 if (prio != 0) {
 errno = EINVAL;
 return -1;
 }
 break;
 case SCHED_FIFO:
 if (prio < SCHED_PRIO_MIN || prio > SCHED_PRIO_MAX) {
 errno = EINVAL;
 return -1;
 }
 break;
 default:
 errno = EINVAL;
 return -1;
 }

 t->set_realtime_priority(prio);
 return 0;
}

int sched_setscheduler(pid_t pid, int policy,
 const struct sched_param *param)
{
 int res = -1;

 if (!param) {
 errno = EINVAL;
 return -1;
 }

 if (pid == 0) {
 sched::thread *t = sched::thread::current();

 res = sched_setparam_aux(t, policy, param->sched_priority);
 } else {
 sched::with_thread_by_id(pid, [param, policy, &res] (sched::thread *t) {
 if (t) res = sched_setparam_aux(t, policy, param->sched_priority);
 });
 if (res == -1) {
 errno = ESRCH;
 }
 }

 return res;
}

int sched_getscheduler(pid_t pid)
{
 int res = -1;

 if (pid == 0) {
 sched::thread *t = sched::thread::current();

 if (t->realtime_priority() == 0) {
 res = SCHED_OTHER;
 } else {
 res = SCHED_FIFO;
 };
 } else {
 sched::with_thread_by_id(pid, [&res] (sched::thread *t) {
 if (t) {
 if (t->realtime_priority() == 0) {
 res = SCHED_OTHER;
 } else {
 res = SCHED_FIFO;
 }
 }
 });
 if (res == -1) {
 errno = ESRCH;
 }
 }

 return res;
}

int sched_getparam(pid_t pid, struct sched_param *param)
{
 int res = 0;

 if (pid == 0) {
 sched::thread *t = sched::thread::current();

 param->sched_priority = t->realtime_priority();
 } else {
 sched::with_thread_by_id(pid, [param, &res] (sched::thread *t) {
 if (t) {
 param->sched_priority = t->realtime_priority();
 } else {
 res = -1;
 }
 });
 if (res == -1) {
 errno = ESRCH;
 }
 }

 return res;
}