RISCV_PREFIX ?= riscv64-unknown-elf-
#RISCV_PREFIX ?= riscv-nuclei-elf-

override CC = $(RISCV_PREFIX)gcc
override CXX = $(RISCV_PREFIX)gcc
override LD = $(RISCV_PREFIX)gcc

ASFLAGS ?= $(CFLAGS)
SIM_TARGET ?= sim-default

# ISA Version 2.2 is the last version where zicsr is still
# supported as part of the base ISA. Use that for now until
# _zicsr is widely supported by existing cross-compilers.
SUPPORTS_MISA ?= $(shell echo "typedef int dont_be_pedantic;" | $(RISCV_PREFIX)gcc -march=rv32imac -mabi=ilp32 -misa-spec=2.2 -E - > /dev/null 2>&1 && echo 1 || echo 0)
ifeq (1,$(SUPPORTS_MISA))
  CFLAGS += -misa-spec=2.2
  CXXFLAGS += -misa-spec=2.2
endif

VP ?= gd32-vp
VP_FLAGS ?= --intercept-syscalls


	#-Wall -g -Os -s -z norelro -Wl,--gc-sections \

BUILD_FLAGS = \
	-march=rv32imac -mabi=ilp32 -mcmodel=medlow \
	-Wall -g3 -ffunction-sections -fdata-sections -fno-common \
	-I include \
	-DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS \
	-DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON \
	-DTF_LITE_MCU_DEBUG_LOG -DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS \
	-DTF_LITE_USE_GLOBAL_MIN -DTF_LITE_USE_GLOBAL_MAX \
	-I tflite-micro \
	-I tflite-micro/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include \
	-I tflite-micro/tensorflow/lite/micro/tools/make/downloads/gemmlowp \
	-I nuclei-sdk/SoC/gd32vf103/Common/Include \
	-I nuclei-sdk/NMSIS/Core/Include \
	-I nuclei-sdk/SoC/gd32vf103/Board/gd32vf103v_eval/Include

CFLAGS += $(BUILD_FLAGS)
CXXFLAGS += $(BUILD_FLAGS) #-std=c++2a
LDFLAGS = -nostartfiles \
	  -Wl,--no-relax -Wl,--gc-sections \
	  -Tnuclei-sdk/SoC/gd32vf103/Board/gd32vf103v_eval/Source/GCC/gcc_gd32vf103_flashxip.ld

EXECUTABLE = main
HEADERS := $(wildcard include/*.h)
ASMSOURCES := $(wildcard nuclei-sdk/SoC/gd32vf103/Common/Source/GCC/*.S)
CSOURCES := $(wildcard src/*.c) $(wildcard nuclei-sdk/SoC/gd32vf103/Common/Source/*.c) $(wildcard nuclei-sdk/SoC/gd32vf103/Common/Source/Drivers/*.c) $(wildcard nuclei-sdk/SoC/gd32vf103/Board/gd32vf103v_eval/Source/*.c)
CPPSOURCES := $(wildcard src/*.cpp)
CCSOURCES := $(wildcard src/*.cc)
OBJECTS := $(patsubst %.S,%.o,$(ASMSOURCES)) $(patsubst %.c,%.o,$(CSOURCES)) $(patsubst %.cpp,%.o,$(CPPSOURCES)) $(patsubst %.cc,%.o,$(CCSOURCES))
LDLIBS := tflite-micro/gen/riscv32_mcu_x86_64_default/lib/libtensorflow-microlite.a -lgcc -lm -lstdc++

########################################################################

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(LD) $(CXXFLAGS) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

########################################################################

sim: $(SIM_TARGET)
sim-default: $(EXECUTABLE)
	$(VP) $(VP_FLAGS) $<

dump-elf: $(EXECUTABLE)
	$(RISCV_PREFIX)readelf -a main

dump-code: $(EXECUTABLE)
	$(RISCV_PREFIX)objdump -D main

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(CLEAN_EXTRA)

.PHONY: all sim sim-default dump-elf dump-code clean

main:
