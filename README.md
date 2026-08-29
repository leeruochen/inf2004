# INF2004 Lab Repository

This repository contains all the labs for the INF2004 module, consolidated into a single workspace. Each lab is placed into its respective sub-folder and covers different aspects of embedded systems and microcontroller programming using the Raspberry Pi Pico.

## Table of Contents

- [LAB 1: Microcontroller and its Development Environment](./LAB1/)
- [LAB 2: GPIO and Digital Communication](./LAB2/)
- [LAB 3: Interrupts & Timers](./LAB3/)
- [LAB 4: PWM & ADCs](./LAB4/)
- [LAB 5: FreeRTOS & Real-Time Concepts](./LAB5/)
- [LAB 6: OPTIMISATION & DEBUGGING](./LAB6/)
- [GUIDE: Device Driver Development](./GUIDE_DeviceDriver/)
- [GUIDE: The Bug Hunts - A Debugging Method](./BUGHUNT.md)

## Lab Descriptions

### [LAB 1: Microcontroller and its Development Environment](./LAB1/)
Familiarise yourself with the development environment for the Raspberry Pi Pico W, including setting up the Pico C/C++ SDK and getting started with the board.

### [LAB 2: GPIO and Digital Communication](./LAB2/)
Learn to configure General Purpose Input/Output (GPIO) pins as inputs and outputs, and understand how to use library functions for digital communication.

### [LAB 3: Interrupts & Timers](./LAB3/)
Explore the differences between polling and interrupts, and learn how to configure and implement hardware interrupts and timers.

### [LAB 4: PWM & ADCs](./LAB4/)
Configure and implement Pulse Width Modulation (PWM) and Analog-to-Digital Converters (ADC) on the Raspberry Pi Pico.

### [LAB 5: FreeRTOS & Real-Time Concepts](./LAB5/)
Configure and implement FreeRTOS on the Raspberry Pi Pico: task states and the preemptive scheduler, periodic tasks without drift, inter-task communication with message buffers and queues, mutual exclusion and priority inversion, deferred interrupt handling, and a sensor filtering algorithm.

### [LAB 6: Optimisation & Debugging](./LAB6/)
Implement various code optimisation methods and configure/use a debugger for the Raspberry Pi Pico to effectively troubleshoot embedded applications.

### [GUIDE: Device Driver Development](./GUIDE_DeviceDriver/)
A comprehensive guide and reference implementation for writing efficient, interrupt-driven device drivers on embedded systems.

---

## [THE BUG HUNTS: A Debugging Method](./BUGHUNT.md)

Every lab above has a **Bug Hunt** attached to it — a small piece of real
firmware with defects already planted in it, and a brief that asks you to find
them. The algorithms are the ones firmware actually contains: bit manipulation,
serialisation, state machines, fixed-point filters, ring buffers, CRCs. But the
algorithm is the excuse. The method is the lesson.

Difficulty escalates deliberately, and not by adding lines of code. Each rung
takes away an instrument the previous rung relied on and hands over a sharper
one.

| # | Lab | Algorithm | Defects | What makes it hard |
|---|---|---|---|---|
| [1](./LAB1/bughunt/) | Environment | Bit counting & manipulation | 6 | It won't compile — and the error points at the wrong line |
| [2](./LAB2/bughunt/) | GPIO & UART | Serialisation & framing | 7 | The symptom is on the other board from the cause |
| [3](./LAB3/bughunt/) | Interrupts & timers | Edge state machine, debounce | 8 | It works in Debug and hangs in Release |
| [4](./LAB4/bughunt/) | PWM & ADC | Fixed-point IIR filter | 8 | The maths is right and the types are wrong |
| [5](./LAB5/bughunt/) | FreeRTOS | Ring buffer & moving average | 9 | It fails once, an hour in, in a different task |
| [6](./LAB6/bughunt/) | Optimisation & debugging | CRC-8, LFSR, frame parser | 12 | Only the disassembly tells the truth |

**Guidance decreases on purpose.** Hunt #1 walks you through the first defect and
hints at every remaining one. Hunt #6 gives you a specification, a number, and
nothing else. Hunt #6 then ends where the original Lab 6 exercise begins — with
[`pid.c`](./LAB6/pid.c), which by then should take you a fraction of the time it
would have in week one.

Read [**BUGHUNT.md**](./BUGHUNT.md) before Hunt #1. It is the method you will use
for all six, and it does not get shorter.
