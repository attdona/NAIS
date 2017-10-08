# name of your application
APPLICATION = hello-proto

# If no BOARD is found in the environment, use this default:
BOARD ?= cc3200-launchxl

BURBA ?= $(CURDIR)/../..
RIOTBASE ?= $(BURBA)/RIOT

DIRS += $(CURDIR)/../../sys
INCLUDES += -I$(BURBA)/sys/protobuf-c


# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -g -DDEVELHELP -DCPUID_ID_LEN=1
CFLAGS_OPT = -O0


# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += bocia

include $(BURBA)/Makefile.include
