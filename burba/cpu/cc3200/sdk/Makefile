DIRS += simplelink simplelink/source oslib driverlib

# When driverlib rom will be supported by TI
#DIRS += simplelink simplelink/source oslib

include $(RIOTBASE)/Makefile.base

# TI cc3200-sdk is not compliant with following RIOT constraints
CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-strict-prototypes -Wno-old-style-definition -Wno-parentheses 
