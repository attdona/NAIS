/*
 * Copyright (C) 2016 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 *
 * @file
 *
 *
 * @defgroup     <id> <visible title>
 * @{
 *
 * @brief
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 */
#include "xtimer.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mutex.h"
#include "thread.h"
#include "irq.h"
#include "div.h"

#include "timex.h"

#define ENABLE_DEBUG 0
#include "debug.h"

int _xtimer_set_absolute64(xtimer_t *timer, uint64_t ticks_offset);

static xtimer_t *timer_list_head = NULL;

static volatile int _in_handler = 0;

static uint32_t wrap_counter = 0;

static void _callback_unlock_mutex(void* arg)
{
    mutex_t *mutex = (mutex_t *) arg;
    mutex_unlock(mutex);
}

void _xtimer_tsleep(uint32_t offset, uint32_t long_offset)
{
    if(irq_is_in()) {
        assert(!long_offset);
        _xtimer_spin(offset);
        return;
    }

    xtimer_t timer;
    mutex_t mutex = MUTEX_INIT;

    timer.callback = _callback_unlock_mutex;
    timer.arg = (void*) &mutex;
    timer.target = timer.long_target = 0;

    mutex_lock(&mutex);
    _xtimer_set64(&timer, offset, long_offset);
    mutex_lock(&mutex);
}

static inline int _is_set(xtimer_t *timer)
{
    return (timer->target);
}

static uint64_t _time_left(xtimer_t* timer, uint32_t reference) {
    uint32_t now = _xtimer_now();
    uint64_t timer_val;

    if(now < reference) {
        return 0;
    }

    timer_val = ((uint64_t) timer->long_target << 32) + timer->target;
    if(timer_val > now) {
        return timer_val - now;
    }
    else {
        return 0;
    }
}

static inline void _lltimer_set(uint32_t target) {

    if(_in_handler) {
        return;
    }

    DEBUG("_lltimer_set(): setting %" PRIu32 "\n", target);
    timer_set_absolute(XTIMER_DEV, XTIMER_CHAN, target);
}

typedef struct priv_xtimer {
    struct xtimer *next; /**< reference to next timer in timer lists */
    uint64_t target64; /**< lower 32bit absolute target time */
    xtimer_callback_t callback; /**< callback function to call when timer
     expires */
    void *arg; /**< argument to pass to callback function */
} priv_xtimer_t;


static void _add_timer_to_list(xtimer_t **list_head, xtimer_t *timer) {

    int state = irq_disable();



     while(*list_head &&
     ((*list_head)->long_target < timer->long_target)) {
     list_head = &((*list_head)->next);
     }

    /*
    while(*list_head &&
            (((priv_xtimer_t*) (*list_head))->target64 <= ((priv_xtimer_t*) timer)->target64)) {
        list_head = &((*list_head)->next);
    }
     */

    while(*list_head &&
            ((*list_head)->target <= timer->target) &&
            ((*list_head)->long_target <= timer->long_target))
    {
        list_head = &((*list_head)->next);
    }


    timer->next = *list_head;
    *list_head = timer;

    if(timer_list_head == timer) {
        if(timer->long_target == 0) {
            _lltimer_set(timer->target);
        }
    }
    irq_restore(state);

}

static int _remove_timer_from_list(xtimer_t **list_head, xtimer_t *timer)
{
    while(*list_head) {
        if(*list_head == timer) {
            *list_head = timer->next;
            return 1;
        }
        list_head = &((*list_head)->next);
    }

    return 0;
}

static void _shoot(xtimer_t *timer) {
    timer->callback(timer->arg);
}

void _xtimer_set(xtimer_t *timer, uint32_t offset)
{
    DEBUG("timer_set(): offset=%" PRIu32 " now=%" PRIu32 " (%" PRIu32 ")\n",
            offset, xtimer_now().ticks32, _xtimer_lltimer_now());
    if(!timer->callback) {
        DEBUG("timer_set(): timer has no callback.\n");
        return;
    }

    xtimer_remove(timer);

    if(offset < XTIMER_BACKOFF) {
        _xtimer_spin(offset);
        _shoot(timer);
    }
    else {
        uint64_t target = (uint64_t) (_xtimer_now()) + offset;
        _xtimer_set_absolute64(timer, target);
    }
}


/**
 * @brief main xtimer callback function
 */
static void _timer_callback(void) {
    uint32_t reference;
    uint32_t next_target = 0;

    _in_handler = 1;

#if DEBUG
    if (!timer_list_head) {
        DEBUG("ERR timer_callback(): no timer to fire\n");
        return;
    }
#endif

    reference = _xtimer_now();

    in_the_past:
    /* check if next timers are close to expiring */
    while(timer_list_head
            && (_time_left(timer_list_head, reference)
                    < XTIMER_ISR_BACKOFF)) {

        /* make sure we don't fire too early */
        while(_time_left(timer_list_head, reference))
            ;

        /* pick first timer in list */
        xtimer_t *timer = timer_list_head;

        /* advance list */
        timer_list_head = timer->next;

        /* make sure timer is recognized as being already fired */
        timer->target = 0;

        /* fire timer */
        _shoot(timer);
    }

    reference = _xtimer_now();

    if(timer_list_head && (timer_list_head->long_target == 0)) {

        /* schedule callback on next timer target time */
        next_target = timer_list_head->target;

        if(reference > next_target)
            goto in_the_past;

        _in_handler = 0;

        /* set low level timer */
        _lltimer_set(next_target);
    }

    _in_handler = 0;

}

static inline void _shoot_now(xtimer_t *timer) {
    /* make sure timer is recognized as being already fired */
    timer->target = 0;

    /* fire timer */
    _shoot(timer);

    /* advance timer list */
    timer_list_head = timer->next;

}

/**
 * timer has reached the top value
 */
static inline void _timer_wrap(void) {
    xtimer_t *list_head;
    uint32_t reference;
    uint8_t scheduled = 0;

    wrap_counter++;

    list_head = timer_list_head;
    while(list_head) {
        reference = _xtimer_now();

        assert(list_head->long_target);
        /*
        if(list_head->long_target) {
            list_head->long_target = list_head->long_target - 1;
        }
         */
        list_head->long_target = list_head->long_target - 1;

        if(scheduled) {
            list_head = list_head->next;
            continue;
        }

        /**
         * check if there are timers in the past or near future that needs
         * to be fired
         */
        if((list_head->long_target == 0) && (reference > list_head->target)) {
            /**
             * timer is in the past, fire it immediately
             */
            _shoot_now(list_head);

        }
        else {

            if((list_head->long_target == 0)) {

                /* set low level timer */
                _lltimer_set(timer_list_head->target);
            }

            scheduled = 1;
        }

        // assert(list_head->target >= ((uint64_t)(1)<<32));

        list_head = list_head->next;
    }

}

static inline void _periph_timer_callback(void *arg, int chan) {
    (void) arg;
    if(chan) {
        _timer_wrap();
    }
    else {
        _timer_callback();
    }
}

static void _callback_msg(void * arg) {
    msg_t *msg = (msg_t *) arg;
    msg_send_int(msg, msg->sender_pid);
}

/**
 * @brief get the current system time as 64bit microsecond value
 *
 * @return  current time as 64bit microsecond value
 */
uint64_t _xtimer_now64(void) {

    return ((uint64_t) wrap_counter << 32) + _xtimer_now();
}

/**
 * @brief get the current system time into a timex_t
 *
 * @param[out] out  pointer to timex_t the time will be written to
 */
void xtimer_now_timex(timex_t *out) {
    uint64_t now = _xtimer_now64();

    out->seconds = now / XTIMER_HZ;
    out->microseconds = now / XTIMER_USEC_TO_TICKS_FACTOR - (out->seconds * 1000000);

}

/**
 * @brief xtimer initialization function
 *
 * This sets up xtimer. Has to be called once at system boot.
 * If @ref auto_init is enabled, it will call this for you.
 */
void xtimer_init(void) {

    /* initialize low-level timer */
    timer_init(XTIMER_DEV, XTIMER_USEC_TO_TICKS(1000000ul),
            _periph_timer_callback, NULL);

}

int inline _xtimer_set_absolute(xtimer_t *timer, uint32_t ticks_offset)
{
    // internal units are clock ticks
    timer->target = ticks_offset - XTIMER_OVERHEAD;
    timer->long_target = 0;

    _add_timer_to_list(&timer_list_head, timer);
    return 0;

}

int _xtimer_set_absolute64(xtimer_t *timer, uint64_t ticks_offset)
{
    // internal units are clock ticks
    uint64_t corr_ticks = ticks_offset - XTIMER_OVERHEAD;
    timer->target = corr_ticks;
    timer->long_target = (corr_ticks >> 32);

    _add_timer_to_list(&timer_list_head, timer);
    return 0;

}


void inline _xtimer_set64(xtimer_t *timer, uint32_t offset,
        uint32_t long_offset) {
    uint32_t now = _xtimer_now();

    uint64_t val64;

    // internal units are clock ticks
    //_xtimer_set_absolute(timer, now + offset);

    // internal units are clock ticks
    val64 = (uint64_t) now + offset - XTIMER_OVERHEAD;
    timer->target = val64;
    timer->long_target = (val64 >> 32) + long_offset;

    _add_timer_to_list(&timer_list_head, timer);

}

/*
 void _xtimer_usleep64(uint64_t offset) {

 if (irq_is_in()) {
 _xtimer_spin(offset);
 return;
 }

 xtimer_t timer;
 mutex_t mutex = MUTEX_INIT;

 timer.callback = _callback_unlock_mutex;
 timer.arg = (void*) &mutex;
 timer.target = 0;

 mutex_lock(&mutex);
 _xtimer_set64(&timer, offset);
 mutex_lock(&mutex);

 }
 */

/**
 * @brief will cause the calling thread to be suspended until the absolute
 * time (@p last_wakeup + @p period).
 *
 * When the function returns, @p last_wakeup is set to
 * (@p last_wakeup + @p period).
 *
 * This function can be used to create periodic wakeups.
 * @c last_wakeup should be set to xtimer_now() before first call of the
 * function.
 *
 * If the result of (@p last_wakeup + @p period) would be in the past, the function
 * sets @p last_wakeup to @p last_wakeup + @p period and returns immediately.
 *
 * @param[in] last_wakeup   base time stamp for the wakeup
 * @param[in] period        time in microseconds that will be added to last_wakeup
 */
void _xtimer_periodic_wakeup(uint32_t *last_wakeup, uint32_t period) {
    xtimer_t timer;
    mutex_t mutex = MUTEX_INIT;

    timer.callback = _callback_unlock_mutex;
    timer.arg = (void*) &mutex;

    uint32_t target = (*last_wakeup) + period;
    uint32_t now = _xtimer_now();
    /* make sure we're not setting a value in the past */
    if(now < (*last_wakeup)) {
        /* base timer overflowed between last_wakeup and now */
        if(!((now < target) && (target < (*last_wakeup)))) {
            /* target time has already passed */
            goto out;
        }
    }
    else {
        /* base timer did not overflow */
        if((((*last_wakeup) <= target) && (target <= now))) {
            /* target time has already passed */
            goto out;
        }
    }

    /*
     * For large offsets, set an absolute target time.
     * As that might cause an underflow, for small offsets, set a relative
     * target time.
     * For very small offsets, spin.
     */
    /*
     * Note: last_wakeup _must never_ specify a time in the future after
     * _xtimer_periodic_sleep returns.
     * If this happens, last_wakeup may specify a time in the future when the
     * next call to _xtimer_periodic_sleep is made, which in turn will trigger
     * the overflow logic above and make the next timer fire too early, causing
     * last_wakeup to point even further into the future, leading to a chain
     * reaction.
     *
     * tl;dr Don't return too early!
     */
    uint32_t offset = target - now;
    DEBUG("xps, now: %9" PRIu32 ", tgt: %9" PRIu32 ", off: %9" PRIu32 "\n", now, target, offset);
    if(offset < XTIMER_PERIODIC_SPIN) {
        _xtimer_spin(offset);
    }
    else {
        if(offset < XTIMER_PERIODIC_RELATIVE) {
            /* NB: This will overshoot the target by the amount of time it took
             * to get here from the beginning of xtimer_periodic_wakeup()
             *
             * Since interrupts are normally enabled inside this function, this time may
             * be undeterministic. */
            target = _xtimer_now() + offset;
        }
        mutex_lock(&mutex);
        DEBUG("xps, abs: %" PRIu32 "\n", target);
        _xtimer_set_absolute(&timer, target);
        mutex_lock(&mutex);
    }

    out:
    *last_wakeup = target;

}

/**
 * @brief Set a timer that sends a message
 *
 * This function sets a timer that will send a message @p offset microseconds
 * from now.
 *
 * The mesage struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
void _xtimer_set_msg(xtimer_t *timer, uint32_t offset, msg_t *msg,
        kernel_pid_t target_pid) {
    _xtimer_set_msg64(timer, offset, msg, target_pid);
}

/**
 * @brief Set a timer that sends a message, 64bit version
 *
 * This function sets a timer that will send a message @p offset microseconds
 * from now.
 *
 * The message struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
void _xtimer_set_msg64(xtimer_t *timer, uint64_t offset, msg_t *msg,
        kernel_pid_t target_pid) {

    offset = _xtimer_now() + offset - XTIMER_OVERHEAD;
    timer->target = offset;

    if(timer->target < offset) {
        timer->long_target = 1;
    }

    //timer->target = _xtimer_now() + offset - XTIMER_OVERHEAD;


    timer->callback = _callback_msg;

    msg->sender_pid = target_pid;

    timer->arg = msg;

    _add_timer_to_list(&timer_list_head, timer);

    // TODO
}

static void _callback_wakeup(void* arg)
{
    thread_wakeup((kernel_pid_t) ((intptr_t) arg));
}

/**
 * @brief Set a timer that wakes up a thread
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
void _xtimer_set_wakeup(xtimer_t *timer, uint32_t offset, kernel_pid_t pid) {

    timer->callback = _callback_wakeup;
    timer->arg = (void*) ((intptr_t) pid);

    _xtimer_set(timer, offset);

}

/**
 * @brief Set a timer that wakes up a thread, 64bit version
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
void _xtimer_set_wakeup64(xtimer_t *timer, uint64_t offset, kernel_pid_t pid) {

    timer->callback = _callback_wakeup;
    timer->arg = (void*) ((intptr_t) pid);

    _xtimer_set64(timer, offset, offset >> 32);

}

static void _remove(xtimer_t *timer)
{
    int state = irq_disable();

    if(timer_list_head == timer) {
        uint32_t next;
        timer_list_head = timer->next;
        if(timer_list_head) {
            /* schedule callback on next timer target time */
            next = timer_list_head->target;
            _lltimer_set(next);
        }

    }
    else {
        _remove_timer_from_list(&timer_list_head, timer);
    }
    irq_restore(state);

}

/**
 * @brief remove a timer
 *
 * @note this function runs in O(n) with n being the number of active timers
 *
 * @param[in] timer ptr to timer structure that will be removed
 */
void xtimer_remove(xtimer_t *timer) {
    if(_is_set(timer)) {
        _remove(timer);
    }

}

/* Prepares the message to trigger the timeout.
 * Additionally, the xtimer_t struct gets initialized.
 */
static void _setup_timer_msg(msg_t *m, xtimer_t *t)
{
    m->type = MSG_XTIMER;
    m->content.ptr = m;

    t->target = 0;
}

/* Waits for incoming message or timeout. */
static int _msg_wait(msg_t *m, msg_t *tmsg, xtimer_t *t)
{
    msg_receive(m);
    if(m->type == MSG_XTIMER && m->content.ptr == tmsg) {
        /* we hit the timeout */
        return -1;
    }
    else {
        xtimer_remove(t);
        return 1;
    }
}

/**
 * @brief receive a message blocking but with timeout, 64bit version
 *
 * @param[out]   msg    pointer to a msg_t which will be filled in case of no
 *                      timeout
 * @param[in]    us     timeout in microseconds relative
 *
 * @return       < 0 on error, other value otherwise
 */
int _xtimer_msg_receive_timeout64(msg_t *msg, uint64_t timeout_ticks) {
    msg_t tmsg;
    xtimer_t t;
    _setup_timer_msg(&tmsg, &t);
    xtimer_set_msg64(&t, timeout_ticks, &tmsg, sched_active_pid);
    return _msg_wait(msg, &tmsg, &t);
}

/**
 * @brief receive a message blocking but with timeout
 *
 * @param[out]  msg     pointer to a msg_t which will be filled in case of
 *                      no timeout
 * @param[in]   us      timeout in microseconds relative
 *
 * @return       < 0 on error, other value otherwise
 */
int _xtimer_msg_receive_timeout(msg_t *msg, uint32_t timeout_ticks) {
    msg_t tmsg;
    xtimer_t t;
    _setup_timer_msg(&tmsg, &t);
    _xtimer_set_msg(&t, timeout_ticks, &tmsg, sched_active_pid);
    return _msg_wait(msg, &tmsg, &t);
}
