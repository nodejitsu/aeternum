/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <pthread.h>
#include <assert.h>
#include <errno.h>


int uv_thread_join(uv_thread_t *tid) {
  if (pthread_join(*tid, NULL))
    return -1;
  else
    return 0;
}


int uv_mutex_init(uv_mutex_t* mutex) {
#ifdef NDEBUG
  if (pthread_mutex_init(mutex, NULL))
    return -1;
  else
    return 0;
#else
  pthread_mutexattr_t attr;
  int r;

  if (pthread_mutexattr_init(&attr))
    abort();

  if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))
    abort();

  r = pthread_mutex_init(mutex, &attr);

  if (pthread_mutexattr_destroy(&attr))
    abort();

  return r ? -1 : 0;
#endif
}


void uv_mutex_destroy(uv_mutex_t* mutex) {
  if (pthread_mutex_destroy(mutex))
    abort();
}


void uv_mutex_lock(uv_mutex_t* mutex) {
  if (pthread_mutex_lock(mutex))
    abort();
}


int uv_mutex_trylock(uv_mutex_t* mutex) {
  int r;

  r = pthread_mutex_trylock(mutex);

  if (r && r != EBUSY && r != EAGAIN)
    abort();

  if (r)
    return -1;
  else
    return 0;
}


void uv_mutex_unlock(uv_mutex_t* mutex) {
  if (pthread_mutex_unlock(mutex))
    abort();
}


int uv_rwlock_init(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_init(rwlock, NULL))
    return -1;
  else
    return 0;
}


void uv_rwlock_destroy(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_destroy(rwlock))
    abort();
}


void uv_rwlock_rdlock(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_rdlock(rwlock))
    abort();
}


int uv_rwlock_tryrdlock(uv_rwlock_t* rwlock) {
  int r;

  r = pthread_rwlock_tryrdlock(rwlock);

  if (r && r != EBUSY && r != EAGAIN)
    abort();

  if (r)
    return -1;
  else
    return 0;
}


void uv_rwlock_rdunlock(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_unlock(rwlock))
    abort();
}


void uv_rwlock_wrlock(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_wrlock(rwlock))
    abort();
}


int uv_rwlock_trywrlock(uv_rwlock_t* rwlock) {
  int r;

  r = pthread_rwlock_trywrlock(rwlock);

  if (r && r != EBUSY && r != EAGAIN)
    abort();

  if (r)
    return -1;
  else
    return 0;
}


void uv_rwlock_wrunlock(uv_rwlock_t* rwlock) {
  if (pthread_rwlock_unlock(rwlock))
    abort();
}


void uv_once(uv_once_t* guard, void (*callback)(void)) {
  if (pthread_once(guard, callback))
    abort();
}

#if defined(__APPLE__) && defined(__MACH__)

int uv_sem_init(uv_sem_t* sem, unsigned int value) {
  if (semaphore_create(mach_task_self(), sem, SYNC_POLICY_FIFO, value))
    return -1;
  else
    return 0;
}


void uv_sem_destroy(uv_sem_t* sem) {
  if (semaphore_destroy(mach_task_self(), *sem))
    abort();
}


void uv_sem_post(uv_sem_t* sem) {
  if (semaphore_signal(*sem))
    abort();
}


void uv_sem_wait(uv_sem_t* sem) {
  int r;

  do
    r = semaphore_wait(*sem);
  while (r == KERN_ABORTED);

  if (r != KERN_SUCCESS)
    abort();
}


int uv_sem_trywait(uv_sem_t* sem) {
  mach_timespec_t interval;

  interval.tv_sec = 0;
  interval.tv_nsec = 0;

  if (semaphore_timedwait(*sem, interval) == KERN_SUCCESS)
    return 0;
  else
    return -1;
}

#else /* !(defined(__APPLE__) && defined(__MACH__)) */

int uv_sem_init(uv_sem_t* sem, unsigned int value) {
  return sem_init(sem, 0, value);
}


void uv_sem_destroy(uv_sem_t* sem) {
  if (sem_destroy(sem))
    abort();
}


void uv_sem_post(uv_sem_t* sem) {
  if (sem_post(sem))
    abort();
}


void uv_sem_wait(uv_sem_t* sem) {
  int r;

  do
    r = sem_wait(sem);
  while (r == -1 && errno == EINTR);

  if (r)
    abort();
}


int uv_sem_trywait(uv_sem_t* sem) {
  int r;

  do
    r = sem_trywait(sem);
  while (r == -1 && errno == EINTR);

  if (r && errno != EAGAIN)
    abort();

  return r;
}

#endif /* defined(__APPLE__) && defined(__MACH__) */
