# # Makefile for Little OS

# --- Configuration ---
TARGET_ARCH := i386
KERNEL_ELF := kernel.elf
ISO_FILE := little-os.iso

# --- Directories ---
ROOT_DIR := $(shell pwd)
SRC_DIR := src
INCLUDE_DIR := include
ARCH_SRC_DIR := arch/$(TARGET_ARCH)
BUILD_DIR := build
ISO_DIR := iso

# --- Tools ---
ASM := nasm
CC := gcc
LD := ld
QEMU := qemu-system-i386 # Use the specific target arch

# --- Flags ---
ASMFLAGS := -f elf32 # Output format: 32-bit ELF
CFLAGS := -m32 -std=gnu11 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -g
CFLAGS += -I$(INCLUDE_DIR) # Add include directory path
LDFLAGS := -T link.ld -melf_i386 # Use linker script in root, 32-bit ELF output

# --- Source Files ---
# Find source files in their respective directories
ASM_SOURCES := $(wildcard $(ARCH_SRC_DIR)/*.s)
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)

# --- Object Files ---
# Generate object file paths in the BUILD_DIR
ASM_OBJECTS := $(patsubst $(ARCH_SRC_DIR)/%.s, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# Tell 'make' where to find source files based on target file patterns
vpath %.s $(ARCH_SRC_DIR)
vpath %.c $(SRC_DIR)
vpath %.h $(INCLUDE_DIR)

# Default target
all: $(ISO_FILE)

# --- Build Rules ---

# Rule to link the kernel ELF file
$(KERNEL_ELF): $(OBJECTS) link.ld
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

# Rule to compile assembly files (.s -> .o)
# $<: first prerequisite (.s file)
# $@: target file (.o file)
# The '| $(BUILD_DIR)' part ensures the build directory exists first (order-only prerequisite)
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(ASM) $(ASMFLAGS) $< -o $@

# Rule to compile C files (.c -> .o)
$(BUILD_DIR)/%.o: %.c $(wildcard $(INCLUDE_DIR)/*.h) | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the build directory
# This is an order-only prerequisite for the object file rules
$(BUILD_DIR):
	@echo "Creating build directory $@..."
	@mkdir -p $@

# Rule to create the ISO image
# Depends on the kernel ELF and the GRUB config
$(ISO_FILE): $(KERNEL_ELF) grub.cfg | $(BUILD_DIR)
	@echo "Creating ISO image $@..."
	@echo "  Cleaning/Creating ISO structure..."
	@rm -rf $(ISO_DIR)
	@echo "  DEBUG: Value of ISO_DIR is '$(ISO_DIR)'"
	@echo "  DEBUG: Attempting mkdir -p '$(ISO_DIR)/boot/grub'"
	@mkdir -p $(ISO_DIR)/boot/grub
	@echo "  Copying kernel and GRUB config..."
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/
	@cp grub.cfg $(ISO_DIR)/boot/grub/
	@echo "  Running grub-mkrescue..."
	@grub-mkrescue -o $@ $(ISO_DIR)
	@echo "  Cleaning up ISO structure..."
	@rm -rf $(ISO_DIR)

# --- Utility Targets ---

# Run in QEMU
run: $(ISO_FILE)
	@echo "Running QEMU with $<..."
	$(QEMU) -cdrom $<

# Clean build artifacts
clean:
	@echo "Cleaning project..."
	@rm -f $(KERNEL_ELF) $(ISO_FILE)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)

# Phony targets are not files
.PHONY: all run clean