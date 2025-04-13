# =============================================================================
# Makefile for Little OS
# =============================================================================

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
ISO_DIR := iso_temp # Temporary directory for building ISO contents

# --- Tools ---
# Use := for simple variable assignment (evaluated once)
ASM := nasm
CC := gcc
LD := ld
QEMU := qemu-system-i386
GRUB_MKRESCUE := grub-mkrescue

# --- Flags ---
ASMFLAGS := -f elf32

# CFLAGS:
# -m32: Target 32-bit architecture
# -std=gnu11: Use C11 standard with GNU extensions
# -ffreestanding: Compile for a freestanding environment (no OS)
# -nostdlib: Don't link against standard C library
# -nostdinc: Don't search standard system include directories
# -fno-builtin: Don't assume standard C functions are available as builtins
# -fno-stack-protector: Disable stack smashing protection (requires OS support)
# -Wall -Wextra: Enable most warnings
# -Werror: Treat all warnings as errors
# -g: Generate debugging information
CFLAGS := -m32 -std=gnu11 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -g

# Include directory for our headers
CFLAGS += -I$(INCLUDE_DIR)

# Find GCC's internal include path for freestanding headers (like stdarg.h)
# 1. Get the full path to libgcc.a for the target architecture
# 2. Get the directory part of that path
# 3. Append '/include' to get the include directory
GCC_LIBGCC_PATH := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
GCC_INCLUDE_DIR := $(shell dirname $(GCC_LIBGCC_PATH))/include

# Add GCC's internal include path using -isystem (treats as system headers)
CFLAGS += -isystem $(GCC_INCLUDE_DIR)

# LDFLAGS:
# -T link.ld: Use our custom linker script
# -melf_i386: Specify 32-bit ELF output format for the linker
LDFLAGS := -T link.ld -melf_i386

# --- Source Files ---
# Use wildcard to find all source files in their respective directories
ASM_SOURCES := $(wildcard $(ARCH_SRC_DIR)/*.s)
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)

# --- Object Files ---
# Use patsubst to generate ---
TARGET_ARCH := i386
KERNEL_ELF := kernel.elf
ISO_FILE := little-os.iso

# --- Directories ---
ROOT_DIR := $(shell pwd)
SRC_DIR := src
INCLUDE_DIR := include
ARCH_SRC_DIR := arch/$(TARGET_ARCH)
BUILD_DIR := build
ISO_DIR := iso_temp

# --- Tools ---
ASM := nasm
CC := gcc
LD := ld
QEMU := qemu-system-i386
GRUB_MKRESCUE := grub-mkrescue

# --- Flags ---
ASMFLAGS := -f elf32 -g # Add -g for assembly debug info
# CFLAGS:
# -m32             : Target 32-bit
# -std=gnu11       : Use C11 standard with GNU extensions
# -ffreestanding   : No standard library/host OS assumptions
# -nostdlib        : Don't link standard library
# -nostdinc        : Don't search standard include paths
# -fno-builtin     : Don't assume standard C functions are built-in
# -fno-stack-protector: Disable stack smashing protection (needs OS support)
# -Wall -Wextra    : Enable most warnings
# -Werror          : Treat warnings as errors
# -g               : Generate debugging information
# -I$(INCLUDE_DIR) : Add our include directory to search path
# -isystem <path>  : Add GCC's internal include path for stdarg.h etc.
#                    !!! IMPORTANT: Replace the path below with the output of: !!!
#                    !!! gcc -m32 -print-libgcc-file-name                     !!!
#                    !!! (and navigate to its containing 'include' directory)  !!!
GCC_INTERNAL_INCLUDE_PATH := $(shell dirname `$(CC) -m32 -print-libgcc-file-name`)/include
CFLAGS := -m32 -std=gnu11 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -g
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += -isystem $(GCC_INTERNAL_INCLUDE_PATH)

# LDFLAGS:
# -T link.ld       : Use our linker script
# -melf_i386       : Output 32-bit ELF format
LDFLAGS := -T link.ld -melf_i386

# --- Source Files ---
# Find source files automatically
ASM_SOURCES := $(wildcard $(ARCH_SRC_DIR)/*.s)
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)

# --- Object Files ---
# Generate object file paths in the BUILD_DIR
ASM_OBJECTS := $(patsubst $(ARCH_SRC_DIR)/%.s, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# Tell 'make' where to find source files based on target file patterns
# This allows rules to be simpler (e.g., build/%.o: %.c)
vpath %.s $(ARCH_SRC_DIR)
vpath %.c $(SRC_DIR)
vpath %.h $(INCLUDE_DIR)

# Default target (the first target in the file)
all: $(ISO_FILE)

# --- Build Rules ---

# Rule to link the kernel ELF file
$(KERNEL_ELF): $(OBJECTS) link.ld
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

# Rule to compile assembly files (.s -> .o)
# $< : The first prerequisite (the .s file)
# $@ : The target file (the .o file)
# | $(BUILD_DIR) : Order-only prerequisite - ensure build dir exists first
$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "Assembling $<..."
	$(ASM) $(ASMFLAGS) $< -o $@

# Rule to compile C files (.c -> .o)
# Depends on the .c file and *all* header files found in include/
# This ensures recompilation if any header changes.
$(BUILD_DIR)/%.o: %.c $(wildcard $(INCLUDE_DIR)/*.h) | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the build directory if it doesn't exist
$(BUILD_DIR):
	@echo "Creating build directory $@..."
	@mkdir -p $@

# Rule to create the ISO image using grub-mkrescue
$(ISO_FILE): $(KERNEL_ELF) grub.cfg | $(BUILD_DIR)
	@echo "Creating ISO image $@..."
	@echo "  Preparing temporary ISO directory: $(ISO_DIR)"
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	@echo "  Copying files..."
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf   # Kernel must match grub.cfg path
	@cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg     # Config must be grub.cfg for grub-mkrescue
	@echo "  Running $(GRUB_MKRESCUE)..."
	@$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)
	@echo "  Cleaning up $(ISO_DIR)..."
	@rm -rf $(ISO_DIR)

# --- Utility Targets ---

# Rule to run the OS in QEMU
# -cdrom $<      : Use the target ISO file as a virtual CD
# -boot d        : Tell BIOS to boot from CD-ROM drive 'd' first
# -serial stdio  : Redirect virtual COM1 port to the host terminal
# -d cpu_reset,int: Log CPU resets and interrupt exceptions (for debugging crashes)
# -no-reboot     : Halt QEMU on triple fault instead of rebooting loop
# -no-shutdown   : Keep QEMU window open if guest OS shuts down
run: $(ISO_FILE)
	@echo "Running QEMU with $< (Serial output will be in this terminal)"
	$(QEMU) -cdrom $< -boot d -serial stdio -d cpu_reset,int -no-reboot -no-shutdown

# Rule to clean build artifacts
clean:
	@echo "Cleaning project..."
	@rm -f $(KERNEL_ELF) $(ISO_FILE)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)

# Phony targets (targets that are not actual files)
.PHONY: all run clean