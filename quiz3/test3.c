/* Preemptible user-level threads for Linux */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

#include "coro.h"

#define UNUSED __attribute__((unused))

/* for compatibility */
#define TIMERSIG SIGRTMIN

/* Structure representing a user-level thread's environment.
 * It holds the execution status and context, along with inter-thread
 * communication details.
 */
typedef struct Env {
    int status;          /* Current status of the thread */
    ucontext_t state;    /* Context for saving and restoring thread execution */
    int state_reentered; /* Indicate if the context has been reentered */
    int ipc_sender;      /* Identifier of the thread that sent an IPC message */
    int ipc_value;       /* Value received from an IPC message */
} Env;

/* Define the maximum number of user-level threads.
 * Increase NENV to allow a higher number of concurrent user-level threads.
 */
#define NENV 1024

/* Status codes for the Env */
enum { ENV_UNUSED, ENV_RUNNABLE, ENV_WAITING };

#define ENV_STACK_SIZE 16384

static Env envs[NENV];
static int curenv;

void coro_main(void *); /* provided by user */

/* Define a "successor context" for the purpose of calling env_exit */
static ucontext_t exiter = {0};

/* Preemption timer */
timer_t timer;
const struct itimerspec ts = {
    {0, 0},
    {0, 10000000},
};

static void make_stack(ucontext_t *ucp)
{
    /* If a stack already exists, no need to allocate a new one */
    if (ucp->uc_stack.ss_sp)
        return;

    /* Allocate a new stack using mmap with the specified size and protections
     */
    ucp->uc_stack.ss_sp =
        mmap(NULL, ENV_STACK_SIZE, PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_PRIVATE, -1, 0);
    ucp->uc_stack.ss_size = ENV_STACK_SIZE;
}

int coro_create(coro_entry entry, void *args)
{
    /* Search for an available environment slot */
    int env;
    for (env = 0; env < NENV; env++) {
        if (envs[env].status == ENV_UNUSED) /* Found a free environment */
            break;
    }
    if (env == NENV) /* No free environments available */
        return -1;
    envs[env].status = ENV_WAITING;

    /* Initialize the context for the new user-level thread */
    getcontext(&envs[env].state);
    make_stack(&envs[env].state);
    envs[env].state.uc_link = &exiter;
    makecontext(&envs[env].state, (void (*)(void)) entry, 1, args);

    /* Return the identifier of the newly created user-level thread */
    return env;
}

static void coro_schedule(void)
{
    int attempts = 0;
    while (attempts < NENV) {
        int candidate = (curenv + attempts) % NENV;
        if (envs[candidate].status == ENV_RUNNABLE) {
            curenv = candidate;
            /* Request delivery of TIMERSIG after 10 ms */
            timer_settime(timer, 0, &ts, NULL);
            setcontext(&envs[curenv].state);
        }
        attempts++;
    }
    exit(0);
}

void coro_yield(void)
{
    envs[curenv].state_reentered = 0;
    getcontext(&envs[curenv].state);
    if (envs[curenv].state_reentered++ == 0) {
        /* Context successfully saved; schedule the next user-level thread to
         * run.
	 */
        coro_schedule();
    }
    /* Upon re-entry, simply resume execution */
}

void coro_exit(void)
{
    envs[curenv].status = ENV_UNUSED;
    coro_schedule();
}

void coro_destroy(int env)
{
    envs[env].status = ENV_UNUSED;
}

int coro_getid(void)
{
    return curenv;
}

int coro_recv(int *who)
{
    envs[curenv].status = ENV_WAITING;
    coro_yield();
    if (who)
        *who = envs[curenv].ipc_sender;
    return envs[curenv].ipc_value;
}

void coro_send(int toenv, int val)
{
    while (envs[toenv].status != ENV_WAITING)
        coro_yield();
    envs[toenv].ipc_sender = curenv;
    envs[toenv].ipc_value = val;
    envs[toenv].status = ENV_RUNNABLE;
}

static void preempt(int signum UNUSED,
                    siginfo_t *si UNUSED,
                    void *context UNUSED)
{
    coro_yield();
}

static void enable_preemption(void)
{
    struct sigaction act = {
        .sa_sigaction = preempt,
        .sa_flags = SA_SIGINFO,
    };
    struct sigevent sigev = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo = TIMERSIG,
        .sigev_value.sival_int = 0,
    };

    sigemptyset(&act.sa_mask);
    sigaction(TIMERSIG, &act, NULL);
    timer_create(CLOCK_MONOTONIC, &sigev, &timer);
}

static void init_threads(coro_entry user_main)
{
    curenv = 0;

    getcontext(&exiter);
    make_stack(&exiter);
    makecontext(&exiter, coro_exit, 0);

    coro_create(user_main, NULL);
    setcontext(&envs[curenv].state);
}

int main(int argc UNUSED, char *argv[] UNUSED)
{
    enable_preemption();
    init_threads(coro_main);
    return 0;
}