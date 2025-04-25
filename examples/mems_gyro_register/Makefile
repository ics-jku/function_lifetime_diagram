
CFLAGS = -march=rv32i -mabi=ilp32 -g3 -O0
LDFLAGS = -march=rv32i -mabi=ilp32 -nostartfiles -Wl,--no-relax -g3 -O0 -T ./linker.ld
RISCV_PREFIX ?= riscv64-unknown-elf-

override CC = $(RISCV_PREFIX)gcc
override CXX = $(RISCV_PREFIX)gcc
override LD = $(RISCV_PREFIX)gcc

ASMSOURCES := $(wildcard *.S)
HEADERS := $(wildcard *.h)
CSOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.S,%.o,$(ASMSOURCES)) $(patsubst %.c,%.o,$(CSOURCES))

EXECUTABLE ?= main

$(EXECUTABLE): $(OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^

%.o: %.S
	$(CC) $(CFLAGS) -c $<
%.o: %.c
	$(CC) $(CFLAGS) -c $<
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: clean
.DEFAULT_GOAL := $(EXECUTABLE)