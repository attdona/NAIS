/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bocia_uart.h"
#include "messages.pb-c.h"


#define ENABLE_DEBUG (1)
#include "debug.h"

int main(void) {
  int i = 0;
  // start uart monitor
  bocia_uart_init();

  char *name;

  while (++i < 10) {
    name = malloc(256);
    printf("pointer_addr: %p\n", name);
    free(name);
  }
  printf("OK\n");
  return 0;
}
