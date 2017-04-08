rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

name := $(shell basename $(CURDIR))

dir_source := source
dir_include := include
dir_build := build
dir_kernel_extension := kernel_extension

LIBS := -lctru
LIBDIRS	:= $(CTRULIB)
LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

INCLUDE	:= $(foreach dir,$(LIBDIRS),-I$(dir)/include)

ARCH := -mcpu=mpcore -mfloat-abi=hard
ASFLAGS := -g $(ARCH)
CFLAGS := -Wall -Wextra -MMD -MP -marm $(ASFLAGS) -mtp=soft -fno-builtin -std=c11 -O2 -flto -ffast-math -mword-relocations \
		-fomit-frame-pointer -ffunction-sections -fdata-sections $(INCLUDE) -I$(dir_include) -DARM11 -D_3DS
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -mtp=soft -Wl,--section-start,.text=0x14000000 -Wl,--gc-sections

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
		$(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
		$(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: $(name).cxi

.PHONY: clean
clean:
	@$(MAKE) -C $(dir_kernel_extension) clean
	@rm -rf $(dir_build)

.PHONY: $(dir_kernel_extension)

$(name).cxi: $(dir_build)/$(name).elf
	@makerom -f ncch -rsf rosalina.rsf -nocodepadding -o $@ -elf $<

$(dir_build)/$(name).elf: $(objects)
	$(LINK.o) $(OUTPUT_OPTION) $^ $(LIBPATHS) $(LIBS)

$(dir_build)/kernel_extension.bin: $(dir_kernel_extension)
	@mkdir -p "$(@D)"
	@$(MAKE) -C $<

$(dir_build)/memory.o : CFLAGS += -O3
$(dir_build)/kernel_extension.o: $(dir_build)/kernel_extension.bin

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
include $(call rwildcard, $(dir_build), *.d)
