/* AUTOGENERATED FILE. DO NOT EDIT. */

//=======Test Runner Used To Run Each Test Below=====
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

//=======Automagically Detected Files To Include=====
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>
#include "../../../sys/include/xtimer.h"
#include "debug.h"

//=======External Functions This Runner Calls=====
extern void setUp(void);
extern void tearDown(void);
extern void test_multiply(void);
extern void test_div(void);
extern void test_1_microsecond(void);
extern void test_2_microsecond(void);
extern void test_10_microsecond(void);
extern void test_100_microsecond(void);
extern void test_500_microsecond(void);
extern void test_wrapping(void);
extern void test_xtimer_now64(void);
extern void test_timers_when_counter_wrap(void);
extern void test_timer_set64(void);
extern void test_set_msg(void);

//=======Test Reset Option=====
void resetTest(void);
void resetTest(void)
{
    tearDown();
    setUp();
}

//=======MAIN=====
int main(void)
{
    UnityBegin("test_cases.c");
    /*
    RUN_TEST(test_multiply, 47);
    RUN_TEST(test_div, 79);
    RUN_TEST(test_1_microsecond, 103);
    RUN_TEST(test_2_microsecond, 117);
    RUN_TEST(test_10_microsecond, 134);
    RUN_TEST(test_100_microsecond, 150);
    RUN_TEST(test_500_microsecond, 166);
    RUN_TEST(test_xtimer_now64, 207);

    RUN_TEST(test_wrapping, 207);
     */
    RUN_TEST(test_timer_set64, 207);

    //RUN_TEST(test_timers_when_counter_wrap, 207);

    //RUN_TEST(test_set_msg, 207);

    return (UnityEnd());
}