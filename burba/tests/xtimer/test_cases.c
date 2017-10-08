/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @file
 *
 * @addtogroup      <module_name>
 * @{
 *
 * @file
 * @brief
 *
 * @author          Attilio Dona'
 */

#include "xtimer.h"
#include "unity.h"

#include <inc/hw_memmap.h>
#include <inc/hw_timer.h>


static volatile int fired;
static xtimer_ticks32_t tf;
static xtimer_ticks32_t ti;

static xtimer_ticks64_t tf64;
static xtimer_ticks64_t ti64;

#define ENABLE_DEBUG 1
#include "debug.h"

#define XTIMER_NOW_CALL_OVERHEAD 35

#ifdef MCU_CC3200
void set_hw_timer_counter(uint32_t val) {
    HWREG(TIMERA0_BASE + TIMER_O_TAV) = val;
}
#endif

void setUp(void) {
    fired = 0;
}

void tearDown(void) {
}

void hello(void* arg) {
    tf = xtimer_now();
    fired = 1;
}

void hello64(void* arg) {
    tf64 = xtimer_now64();
    fired = 1;
}

void test_multiply(void) {
    xtimer_ticks32_t t1, t2;
    int a[5] = {
            1,
            10,
            2000,
            10000,
            34566 };
    int b[5] = {
            1,
            80000,
            80000,
            80000000,
            89987765 };
    int64_t x, y;
    int64_t c;

    x = 345666;
    y = 8000000;

    t1 = xtimer_now();
    t2 = xtimer_now();
    DEBUG("naked xtimer_now() extimated time (usec): %ld\n", (t2.ticks32 - t1.ticks32));

    TEST_ASSERT((t2.ticks32 - t1.ticks32) <= XTIMER_NOW_CALL_OVERHEAD);

    for (int i = 0; i < 5; i++) {
        t1 = xtimer_now();
        c = a[i] * b[i];
        t2 = xtimer_now();
        DEBUG("%d*%d=%lld time: %ld\n", a[i], b[i], c, (t2.ticks32 - t1.ticks32));
    }
    t1 = xtimer_now();
    c = x * y;
    t2 = xtimer_now();
    DEBUG("%lld*%lld=%lld time: %ld\n", x, y, c, (t2.ticks32 - t1.ticks32));
    TEST_ASSERT(c == 2765328000000);

    TEST_ASSERT((t2.ticks32 - t1.ticks32) <= XTIMER_NOW_CALL_OVERHEAD);
}

void test_div(void) {
    xtimer_ticks32_t t1, t2;
    int a[5] = {
            1,
            10,
            2000,
            10000,
            34566 };
    int b[5] = {
            1,
            80000,
            80000,
            80000000,
            89987765 };
    int x, y;
    int c;

    x = 345666;
    y = 8000000;

    for (int i = 0; i < 5; i++) {
        t1 = xtimer_now();
        c = b[i] / a[i];
        t2 = xtimer_now();
        DEBUG("%d/%d=%d time: %ld\n", b[i], a[i], c, (t2.ticks32 - t1.ticks32));
    }
    t1 = xtimer_now();
    c = y / x;
    TEST_ASSERT(c == 23);
    t2 = xtimer_now();
    DEBUG("%d/%d=%d time: %ld\n", y, x, c, (t2.ticks32 - t1.ticks32));
    TEST_ASSERT((t2.ticks32 - t1.ticks32) <= XTIMER_NOW_CALL_OVERHEAD);
}

void test_1_microsecond(void) {

    xtimer_t tim;

    tim.callback = hello;

    ti = xtimer_now();
    xtimer_set(&tim, 1); // 1 microsecond

    while (!fired)
        ;
    DEBUG("1us delta time: %ld\n", tf.ticks32 - ti.ticks32);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 5);
}

void test_2_microsecond(void) {

    xtimer_t tim;

    tim.callback = hello;

    ti = xtimer_now();
    xtimer_set(&tim, 2);

    while (!fired)
        ;

    DEBUG("2us delta time: %ld\n", tf.ticks32 - ti.ticks32);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 4);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= 2);

}

void test_10_microsecond(void) {

    xtimer_t tim;

    tim.callback = hello;

    ti = xtimer_now();
    xtimer_set(&tim, 10);

    while (!fired)
        ;

    DEBUG("10us delta time: %ld\n", tf.ticks32 - ti.ticks32);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 10);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= 10);

}

void test_100_microsecond(void) {

    xtimer_t tim;

    tim.callback = hello;

    ti = xtimer_now();
    xtimer_set(&tim, 100);

    while (!fired)
        ;

    DEBUG("100us delta time: %ld\n", tf.ticks32 - ti.ticks32);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 100);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= 100);
}

void test_500_microsecond(void) {

    xtimer_t tim;

    tim.callback = hello;

    ti = xtimer_now();
    xtimer_set(&tim, 500);

    while (!fired)
        ;

    DEBUG("500us delta time: %ld\n", tf.ticks32 - ti.ticks32);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 500);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= 500);

}

#define SLEEP_TIME_US 10000000
void test_10_seconds(void) {
    ti = xtimer_now();
    xtimer_usleep(SLEEP_TIME_US);
    tf = xtimer_now();

    TEST_ASSERT(
            _xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= (SLEEP_TIME_US + 9));

    TEST_ASSERT(
            _xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= SLEEP_TIME_US);
}

#define MAX_COUNTER_VALUE 0xffffffff
#define MAX_US MAX_COUNTER_VALUE/80

void test_xtimer_now64(void) {
    xtimer_ticks64_t ti64, tf64;
    ti64 = xtimer_now64();
    xtimer_usleep(MAX_US);
    tf64 = xtimer_now64();

    TEST_ASSERT(
            _xtimer_usec_from_ticks64(tf64.ticks64 - ti64.ticks64) <= (MAX_US + 9));

    TEST_ASSERT(
            _xtimer_usec_from_ticks64(tf64.ticks64 - ti64.ticks64) >= MAX_US);

}


void test_wrapping(void) {
    xtimer_ticks64_t ti64, tf64;
    ti64 = xtimer_now64();
    xtimer_usleep(MAX_COUNTER_VALUE);
    tf64 = xtimer_now64();

    TEST_ASSERT(
            tf64.ticks64 - ti64.ticks64 <= (uint64_t)MAX_COUNTER_VALUE + 700);

    TEST_ASSERT(tf64.ticks64 - ti64.ticks64 >= MAX_COUNTER_VALUE);

}

static volatile uint32_t check;

void t1_cb(void* arg) {
    check = 1;
}

void t2_cb(void* arg) {
    check = 10 + check * 10;
    fired = 1;
}


#define T0 53687066 // ~ (0xffffffff-2000)/80
#define T1 20       // ~ 1600/80
#define T2 40       // ~ 3200/80
void test_timers_when_counter_wrap(void) {
    xtimer_t t1, t2;

    //HWREG(TIMERA0_BASE + TIMER_O_TAV) = 0xffffffff - 2000;
    set_hw_timer_counter(0xffffffff - 2000);
    check = 0;

    t1.callback = t1_cb;
    xtimer_set(&t1, T1);

    t2.callback = t2_cb;
    xtimer_set(&t2, T2);

    while(!fired)
        ;

    DEBUG("check: %ld\n", check);
    TEST_ASSERT(check == 20);
}

char timer_stack[THREAD_STACKSIZE_MAIN];

void *timer_thread(void *arg)
{
    uint8_t sts = 0;

    /* we need a queue if the second message arrives while the first is still processed */
    /* without a queue, the message would get lost */
    /* because of the way this timer works, there can be max 1 queued message */
    msg_t msgq[1];
    msg_init_queue(msgq, 1);

    (void) arg;
    while(1) {
        msg_t m;
        msg_receive(&m);
        if(sts == 0) {
            if(m.type == 1) {
                sts = 1;
            }
            else {
                fired = 2;
            }
        }
        else {
            if(m.type == 10) {
                fired = 1;
            }
        }
        printf("message: %d\n", m.type);
    }
}

#define TICKS_MARGIN 200000

void test_set_msg(void) {
    xtimer_t t1 =
            { .target = 0, .long_target = 0 };

    xtimer_t t2 =
            { .target = 0, .long_target = 0 };

    msg_t msg1;
    msg1.type = 1;

    msg_t msg2;
    msg2.type = 10;

    kernel_pid_t pid = thread_create(
            timer_stack,
            sizeof(timer_stack),
            THREAD_PRIORITY_MAIN - 1,
            THREAD_CREATE_STACKTEST,
            timer_thread,
            NULL,
            "timer");


    set_hw_timer_counter(0xffffffff - TICKS_MARGIN);

    xtimer_set_msg(&t1, TICKS_MARGIN / (80 * 2), &msg1, pid);
    xtimer_set_msg(&t2, 2000000, &msg2, pid);

    while(!fired)
        ;

    printf("fired: %d\n", fired);
    TEST_ASSERT(fired == 1);
}

void test_long_periods(void) {
    int count = 0;
    printf("long test, please wait 150 secs\n");
    while (count < 150) {
        xtimer_usleep(1000000);
        if (count % 10 == 0) {
            printf("%d secs passed ...\n", count);
        }
        count++;
    }

    TEST_ASSERT(count == 150);

}

void test_timer_set64(void) {
    xtimer_t tim;
    tim.callback = hello;

    ti = xtimer_now();
    _xtimer_set64(&tim, 8000, 0);

    while(!fired)
        ;
    DEBUG("100us delta ticks: %lu\n", tf.ticks32 - ti.ticks32);

    DEBUG("100us delta time: %lu\n",
            _xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32));
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) <= 100);
    TEST_ASSERT(_xtimer_usec_from_ticks(tf.ticks32 - ti.ticks32) >= 99);

    fired = 0;
    tim.callback = hello64;

    ti64 = xtimer_now64();
    _xtimer_set64(&tim, 0, 2);

    while(!fired)
        ;

    DEBUG("53s delta time: %lu\n",
            (uint32_t) _xtimer_usec_from_ticks64(tf64.ticks64 - ti64.ticks64));

    TEST_ASSERT(tf64.ticks64 - ti64.ticks64 <= 0x200000025);
    TEST_ASSERT(tf64.ticks64 - ti64.ticks64 >= 0x200000000);

}
