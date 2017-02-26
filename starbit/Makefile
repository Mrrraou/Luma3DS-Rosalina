rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

name := starbit

dir_source := source
dir_include := include
dir_build := build

ASFLAGS := -mcpu=arm946e-s
CFLAGS := -Wall -Wextra $(ASFLAGS) -fno-builtin -mthumb -mthumb-interwork -std=c11 -Wno-main -O2 -flto -ffast-math -I$(dir_include)
LDFLAGS := -nostartfiles

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
          $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
          $(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: $(name).bin

.PHONY: clean
clean:
	@rm -rf $(dir_build)

$(name).bin: $(dir_build)/main.bin
	@cp -a $(dir_build)/main.bin $@

$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@

$(dir_build)/main.elf: $(objects)
	$(LINK.o) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_build)/memory.o : CFLAGS += -O3 -marm
$(dir_build)/crypto.o : CFLAGS += -O3 -marm

$(dir_build)/%.o: $(dir_source)/%.c $(bundled)
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(OUTPUT_OPTION) $<
include $(call rwildcard, $(dir_build), *.d)
