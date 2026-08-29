# BUG HUNT #6 — Only the disassembly tells the truth

> **Guidance level: none.** A specification, a defect count, and the hardware.
> This is what the previous five were for.

| | |
|---|---|
| **Algorithm** | CRC-8 table, LFSR, frame parser, string formatting |
| **Defects planted** | **12** — then `../pid.c`, which has at least 10 more |
| **Runs on** | Laptop and Pico. Some defects exist on only one of them. |
| **Time** | 90 minutes, plus `pid.c` |
| **You must hand in** | `LOGBOOK.md`, a watchpoint transcript, a fault report |

---

## The specification

At the top of [`bughunt6.c`](bughunt6.c). It is correct. The code is not.

```bash
gcc -Wall -Wextra -O0 -o bughunt6_host bughunt6_host.c bughunt6.c && ./bughunt6_host
```

Then build it again at `-O2`. Then at `-O3`. Then on the Pico, at `Debug` and at
`Release`. **Five builds of identical source, five different behaviours.** Record
all five in your logbook before you form a single hypothesis.

---

## Three required exercises

These are not optional and they are what you are marked on. Each one teaches an
instrument you have not used yet, and each one is the *only* practical way to
find at least one of the twelve.

### A. Find a corruption with a data watchpoint

Somewhere in this program, one variable's value changes without any line of code
appearing to assign to it. You will not find this by reading, and you will not
find it with `printf` — by the time you print the variable, the damage is long
done and the culprit has moved on.

Set a **data watchpoint** (a hardware breakpoint on *write access to an address*,
not on a line of code):

```
(gdb) watch <the variable>
(gdb) continue
```

The Cortex-M0+ has a small number of these in silicon. The debugger will stop the
processor at the exact instruction that performed the write, and you can then
look at the call stack to see who did it.

**Hand in the transcript**: the watchpoint firing, the old and new values, and
the line of code that turned out to be responsible.

> This is the single highest-value debugging technique on this course. An entire
> class of bug — one module quietly writing over another module's memory — is
> essentially unfindable without it, and takes about ninety seconds with it.

### B. Decode a HardFault

One of the defects will fault the processor. When it does, **do not press reset**.
A HardFault is not a crash to be recovered from; it is a report, and the
processor has already written it down for you.

On entry to the fault handler, the Cortex-M0+ has stacked eight registers:
`R0 R1 R2 R3 R12 LR PC xPSR`. The stacked `PC` is the address of the instruction
that faulted.

1. Find the stacked `PC`. (`LR` on fault entry tells you which stack pointer was
   in use — `MSP` or `PSP` — and the stacked frame is at the top of that stack.)
2. Look that address up in the disassembly:
   `arm-none-eabi-objdump -d bughunt6.elf`
3. Identify the instruction and the C line it came from.
4. Explain why *that* instruction is illegal **on this processor**, when the same
   C compiles and runs perfectly on your laptop.

The answer is in the ARMv6-M Architecture Reference Manual, and it is a property
of the Cortex-M0+ that the Cortex-M4 in many other boards does not share. This is
why "it worked on the other dev board" is not evidence of anything.

**Hand in the fault report**: faulting address, instruction, C line, and cause.

### C. Prove an optimisation defect from the disassembly

At least two defects are invisible at `-O0` and fatal at `-O3`. For **one** of
them:

```bash
arm-none-eabi-objdump -d build-debug/bughunt6.elf   > debug.asm
arm-none-eabi-objdump -d build-release/bughunt6.elf > release.asm
```

Find the same function in both. Show the instructions that are present in one and
absent in the other, and explain what the compiler proved in order to justify
removing them.

Then explain why the compiler was entitled to do it. In every case the answer is
the same shape: **you wrote something that the C standard leaves undefined, or
you failed to tell the compiler a fact it had no way to discover.** The compiler
is not your adversary. It is a very literal reader of a contract you did not read
as carefully as it did.

**Hand in the disassembly extracts**, annotated.

---

## Then: `pid.c`

When all twelve are fixed and the harness prints
`TELEMETRY LAYER MATCHES THE SPECIFICATION`, open [`../pid.c`](../pid.c) and do
the original Lab 6 exercise: at least five syntax errors and at least five
logical errors, against the pseudocode in the [lab brief](../README.md).

You have met several of its defects already this semester. One of them is
exactly the `printf` format defect from Bug Hunt #4. Another is a comparison
operator doing something other than what it looks like. You should find them
considerably faster than you would have in week one — and if you do, that
speed *is* the thing this whole ladder was built to give you.

---

## Hand in

- `LOGBOOK.md`, at least **twelve** defect rows plus `pid.c`, plus your
  hypothesis trail — including the wrong hypotheses.
- The watchpoint transcript (exercise A).
- The fault report (exercise B).
- The annotated disassembly extracts (exercise C).
- In the reflection, answer these two:

> 1. Rank the twelve defects by how long each took you to find. Is that ranking
>    correlated with how *serious* each one is? What does your answer imply about
>    where testing effort should go?
>
> 2. Six hunts ago you were fixing a missing semicolon. Describe one thing you now
>    do automatically that you did not do in week one — a habit, a flag, a check,
>    a reflex — and name the defect that taught it to you.
