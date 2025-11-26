# MIPS32 Assembler & Compiler

A custom compiler that translates a high-level hardware control language into MIPS32 assembly code for PIC32 microcontrollers. Built as part of an Analysis of Programming Languages course at TalTech.

## Overview

This project implements a complete compilation pipeline using **Flex** (lexical analysis) and **Bison** (syntax parsing) to generate optimized MIPS32 assembly code for embedded systems programming. The compiler targets PIC32 development boards, enabling intuitive control of LEDs, switches, and other hardware peripherals.

## Features

- **Lexical Analysis**: Flex-based tokenizer for hardware control keywords
- **Syntax Parsing**: Bison-generated parser with error reporting
- **Code Generation**: Translates high-level constructs to MIPS32 assembly
- **Hardware Abstraction**: Simple syntax for GPIO operations (LEDs, switches, RGB)
- **Control Flow**: Support for loops, conditionals (if/else), and delays
- **Optimization**: Efficient register allocation and label management
- **Port Configuration**: Automatic digital I/O initialization for PIC32 ports

## Language Syntax

The compiler accepts a simple, C-like syntax for hardware control:

```c
// Control LED based on switch input with PWM-like behavior
LOOP {
    IF (SW0) {
        LD0 = 1;
        DELAY(75);
        LD0 = 0;
        DELAY(25);
    } ELSE {
        LD0 = 1;
        DELAY(25);
        LD0 = 0;
        DELAY(75);
    }
}
```

### Supported Constructs

| Feature | Syntax | Description |
|---------|--------|-------------|
| **LED Control** | `LD0 = 1;` | Turn LED on (LD0-LD7) |
| **Switch Input** | `LD0 = SW0;` | Map switch to LED (SW0-SW7) |
| **RGB LEDs** | `R = 1; G = 0; B = 1;` | Control RGB components |
| **Loops** | `LOOP { ... }` | Infinite loop |
| **Conditionals** | `IF (SW0) { ... } ELSE { ... }` | Conditional execution |
| **Delays** | `DELAY(100);` | Software delay cycles |

## Architecture

```
┌──────────────┐
│  Source Code │  (.pic files)
│  LD0 = 1;    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Scanner    │  (Flex - scanner.l)
│  Tokenizer   │  Lexical Analysis
└──────┬───────┘
       │
       ▼
┌──────────────┐
│    Parser    │  (Bison - parser.y)
│  AST Builder │  Syntax Analysis
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Code Gen    │  (pic.c)
│ MIPS32 ASM   │  Code Generation
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Output     │  (.s files)
│ lui $8, %hi  │  MIPS32 Assembly
└──────────────┘
```

## Technical Implementation

### 1. Lexical Analysis (`scanner.l`)
- Tokenizes keywords: `LOOP`, `DELAY`, `IF`, `ELSE`
- Recognizes hardware identifiers: `LD[0-7]`, `SW[0-7]`, `R`, `G`, `B`
- Parses numeric literals and operators
- Case-insensitive matching

### 2. Syntax Analysis (`parser.y`)
- Context-free grammar for hardware control statements
- Abstract Syntax Tree (AST) construction
- Symbol table management for GPIO variables
- Error reporting with line numbers

### 3. Code Generation (`pic.c`)
- **Variable Mapping**: Maps logical names (LD0, SW0) to physical ports
  - LEDs → PORTA (bits 0-7)
  - Switches → PORTF, PORTD, PORTB
  - RGB → PORTD
- **Assembly Generation**: Translates AST nodes to MIPS32 instructions
  - `lui`/`sw` for memory-mapped I/O
  - `andi`/`bne` for conditional logic
  - Label generation for control flow
- **Port Initialization**: Configures TRIS registers for I/O direction
- **Board Configuration**: Sets PIC32 config bits

## Build & Usage

### Prerequisites
```bash
gcc
flex (lexical analyzer)
bison (parser generator)
```

### Compilation
```bash
make
```

This generates:
- `scanner.c` from `scanner.l`
- `parser.c` and `parser.h` from `parser.y`
- `pic` executable

### Running the Compiler
```bash
./pic input.pic output.s
```

**Example:**
```bash
./pic test.pic prog.s
```

The compiler will:
1. Parse `test.pic`
2. Generate MIPS32 assembly in `prog.s`
3. Print the AST to stdout

### Clean Build
```bash
make clean    # Remove generated files
make rebuild  # Clean + build
```

## Example Compilation

**Input (`test.pic`):**
```c
LD0 = SW0;
DELAY(1000);
```

**Generated Assembly (excerpt):**
```asm
# assign SW0 to LD0
lui     $8,%hi(PORTF)
lw      $9,%lo(PORTF)($8)
andi    $9,$9,0x8
bne     $9,$0,.lbl1
nop

# else: turn LED off
lui     $8,%hi(LATACLR)
li      $9,0x1
sw      $9,%lo(LATACLR)($8)

# delay 1000
li      $10,0x3e8
.lbl2:
addi    $10,$10,-1
bne     $10,$0,.lbl2
nop
```

## GPIO Mapping

| Component | Port | Bit Mask | Type |
|-----------|------|----------|------|
| LD0-LD7 | PORTA | 0x01-0x80 | Output |
| SW0-SW2 | PORTF | 0x08, 0x20, 0x10 | Input |
| SW3-SW4 | PORTD | 0x8000, 0x4000 | Input |
| SW5-SW7 | PORTB | 0x800, 0x400, 0x200 | Input |
| R (Red) | PORTD | 0x04 | Output |
| G (Green) | PORTD | 0x1000 | Output |
| B (Blue) | PORTD | 0x08 | Output |

## Project Structure

```
.
├── Makefile          # Build configuration
├── scanner.l         # Flex lexer definition
├── parser.y          # Bison grammar definition
├── pic.c             # Main compiler & code generator
├── pic.h             # AST node definitions
├── *.pic             # Test input programs
└── *.s               # Generated MIPS32 assembly
```

## Learning Outcomes

This project demonstrates:

- **Compiler Design**: Complete pipeline from source to assembly
- **Parsing Theory**: CFG implementation with shift-reduce parsing
- **Code Generation**: Target-specific instruction selection
- **Embedded Systems**: Memory-mapped I/O, register manipulation
- **MIPS Architecture**: Load-immediate, branching, delay slots
- **Build Systems**: Makefile with tool chains (flex/bison/gcc)

## Technical Highlights

### AST Node Types
```c
typedef enum {
    tConst,   // Numeric constants
    tVar,     // GPIO variables
    tAssign,  // Assignment statements
    tBlock,   // Statement blocks
    tLoop,    // Infinite loops
    tDelay,   // Software delays
    tIf       // Conditional branches
} NodeType;
```

### Optimization Techniques
- **Register Allocation**: Uses `$8`, `$9`, `$10` for efficient I/O operations
- **Branch Delay Slots**: Properly handles MIPS `nop` instructions
- **Memory-Mapped I/O**: Direct PORT access without function call overhead
- **Label Management**: Unique label generation for control flow

## Future Enhancements

- [ ] Support for arithmetic expressions (`LD0 = SW0 + SW1`)
- [ ] While loops with conditions (`WHILE (SW0) { ... }`)
- [ ] PWM hardware support
- [ ] Function definitions and calls
- [ ] Variable declarations beyond GPIO
- [ ] Optimization passes (dead code elimination, constant folding)

## Course Context

**Course**: Analysis of Programming Languages
**Institution**: TalTech (Tallinn University of Technology)
**Topic**: Compiler Construction, Lexical & Syntax Analysis

---

**Author**: [Liviu]
**License**: Educational Use
