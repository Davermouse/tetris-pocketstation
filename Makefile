ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

PROG=tetris
MCDATA=${PROG}.bin
BUILD_DIR = build

BUILD_DIR ?= .
SOURCE_DIR ?= .

SRCS=locore.S init.c tetris.c shapes.c rand.c

CC=$(DEVKITARM)/bin/arm-none-eabi-gcc
AS=$(DEVKITARM)/bin/arm-none-eabi-as   
LD=$(DEVKITARM)/bin/arm-none-eabi-ld
OBJCOPY=$(DEVKITARM)/bin/arm-none-eabi-objcopy
LDFLAGS=-L/$(DEVKITARM)/lib/gcc/arm-none-eabi/14.2.0/ -lgcc  -Tldscript.ld

CFLAGS= -mcpu=arm7tdmi -mtune=arm7tdmi -O2 -fno-builtin

all: ${MCDATA}

${MCDATA}: ${PROG}
	${OBJCOPY} -O binary ${PROG} ${MCDATA}

${PROG}: $(SRCS:%.c=%.o) $(SRCS:%.S=%.o)
	${LD} -o ${PROG} $(addsuffix .o,$(basename $(SRCS))) ${LDFLAGS}

clean:
	rm -f *.o ${PROG} ${MCDATA}

.PHONY: all clean