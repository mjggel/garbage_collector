# Mark-and-Sweep Garbage Collector

A **Conservative Garbage Collector** written in pure C from scratch.
This project abandons standard abstractions (`malloc`, `free`) to interact directly with the OS Kernel, implementing raw memory management, pointer arithmetic, and stack scanning.

## Features

* **Zero Malloc:** Replaces the standard allocator with direct Kernel memory mapping via `mmap` (Anonymous/Private).
* **Pointer Tagging:** Utilizes 64-bit memory alignment to store `MARKED` and `FREE` flags inside the pointer's unused bits (optimization).
* **Stack Scanning:** Implements inline x86_64 Assembly and `setjmp` exploits to flush CPU registers and scan the stack for root pointers.
* **Coalescing:** Automatic merging of adjacent free blocks (defragmentation) during the sweep phase.
* **Conservative Strategy:** Identifies roots without explicit type information, preventing crashes by assuming "looks like a pointer, is a pointer."

## Core Mechanics

The system manages a raw memory **Arena** sliced into a linked list of **Blocks**.

### Pointer Tagging Strategy

Since `malloc` (and our allocator) aligns memory to 8 bytes on 64-bit systems, the last 3 bits of any valid pointer are always `000`. We use them for metadata:

| Bit | Flag | Description |
| :--- | :--- | :--- |
| **0** | `TAG_MARK` | Object is alive (Mark phase). |
| **1** | `TAG_FREE` | Object is available for reallocation. |
| **2** | `UNUSED` | Available for future flags. |

### API

The library provides a simple interface to manage the heap:

* `gc_init(size_t size, void* stack_top)`: Initializes the Arena requesting pages from the OS.
* `gc_alloc(size_t size)`: Allocates aligned memory (First-Fit strategy).
* `gc_collect()`: Triggers the Mark-and-Sweep process (Stop-the-World).

## How to Build & Run

### Prerequisites

* **OS:** Linux (Requires `mmap` and x86_64 architecture).
* **Compiler:** GCC or Clang.
* **Tools:** Make.

### Compilation

```bash
# 1. Build the main simulation and the test suite
make all

# 2. Run the main server simulation
./gc_project

# 3. Run the stress tests (Cycles & Coalescing)
./test_suite
```

## Development & Testing

The project is split into the core engine and a stress-test suite designed to break the allocator.

## Project Architecture:

**include/:** Header files (Struct definitions and Macros).

**src/:** Core implementation (arena.c) and simulation (main.c).

**tests/:** Edge-case testing (stress_test.c).

**Makefile:** Build automation with debug flags enabled.

### Test Cases
The test_suite validates complex memory scenarios:

* **Coalescing**: Allocates contiguously, frees the middle blocks, and verifies if a larger allocation fits in the merged gap.

* **Circular References**: Creates a "Ouroboros" (A points to B, B points to A) and verifies if the GC collects them when they become unreachable from the Stack.

## Tech Stack
* **Language**: C (Standard C99/C11)

* **System Calls**: Linux Kernel (mmap, munmap)

* **Architecture**: x86_64 Assembly (Inline)

* **Debugging**: GDB & Valgrind-ready

---

**Educational purpose only.**

Developed by **mjggel** — 2026.