# Compiler settings
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# Compiler flags - using only one architecture flag
CFLAGS = -mcpu=cortex-a53 -fpic -ffreestanding -std=gnu99 -O2 -Wall -Wextra
ASFLAGS = -mcpu=cortex-a53
LDFLAGS = -nostdlib

# Source files
C_SOURCES = $(wildcard src/*.c)
ASM_SOURCES = $(wildcard src/*.S)
OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.S=.o)

# Output files
KERNEL = kernel.img

all: $(KERNEL)

# Compile C sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly sources
%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

# Link the kernel
$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) -T src/linker.ld -o kernel.elf $(OBJ)
	$(OBJCOPY) -O binary kernel.elf $(KERNEL)

# Run in QEMU
run: $(KERNEL)
	qemu-system-aarch64 -M virt -cpu cortex-a53 -kernel kernel.img -serial stdio

# Clean
clean:
	rm -f $(OBJ) kernel.elf $(KERNEL)

.PHONY: all clean run