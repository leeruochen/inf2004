# BUG HUNT #3 — Works in Debug, hangs in Release

> **Guidance level: medium.** No walkthrough this time. Instead of hints, you get
> a list of **questions to ask the code** — one per area, and it is your job to
> turn each question into a hypothesis and an experiment. There is exactly one
> hint, and it is sealed for use after thirty minutes of genuine effort.
> Method: [`../../BUGHUNT.md`](../../BUGHUNT.md).

| | |
|---|---|
| **Algorithm** | Edge state machine, debounce, pulse-width timing |
| **Defects planted** | **8** — 4 logical, 3 Heisenbugs, 1 undefined behaviour |
| **Runs on** | Pico W + HC020K IR wheel encoder. No laptop shortcut this week. |
| **Time** | 60–75 minutes |
| **You must hand in** | `LOGBOOK.md` **and** the disassembly evidence from Task A |

---

## The situation

`bughunt3.c` is a wheel-encoder driver: count slots, measure how long each slot
took, debounce the noisy optical edge, report the speed.

It compiles. Read the compiler warnings anyway — **two of the eight defects are
sitting in them**, and this is the last hunt where I will remind you.

---

## This week the build configuration is part of the bug

Everything you have debugged so far behaved the same way every time you ran it.
That ends here.

**Task A, before you change a single character of source:**

First, **predict**. Read `bughunt3.c` and write down in your logbook what you
expect each build to do — before you build either of them. You have been doing
this since Lab 1's operator exercise, and this is the week it starts paying:
the whole point of what follows is that one of your two predictions is wrong,
and the gap between what you expected and what happens is the bug. A prediction
written down afterwards is not a prediction.

Now build the *identical, unmodified* source twice, into two separate
directories:

```bash
mkdir build-debug && cd build-debug
cmake -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=Debug ..    && make    # -O0
cd ..
mkdir build-release && cd build-release
cmake -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=Release ..  && make    # -O3
```

Flash each one, turn the wheel, and record in your logbook **exactly** what each
build does. They are not the same. One of them never prints `done.` at all.

Then go and find out why, using the tools rather than guessing:

```bash
arm-none-eabi-objdump -d build-release/bughunt3.elf > release.asm
arm-none-eabi-objdump -d build-debug/bughunt3.elf   > debug.asm
```

Find `main` in both files. Locate the loop that waits for `pulse_count`.

In the **Debug** build the value is reloaded from memory on every pass:

```
100003f8:  ldr   r3, [r3, #0]     <- read pulse_count
100003fc:  cmp   r3, #19
100003fe:  bls.n 100003f8         <- go back and read it again
```

In the **Release** build the load happens exactly once, before the loop, and
what is left behind is a single instruction that branches to its own address:

```
100003de:  ldr   r3, [r6, #0]     <- read pulse_count, once
100003e0:  cmp   r3, #19
100003e2:  bhi.n 100003e6         <- leave, if it is already big enough
100003e4:  b.n   100003e4         <- this is the entire wait loop
```

Look at that last instruction until it bothers you. `100003e4` branches to
`100003e4`. The compiler did not merely move the load outside the loop — having
satisfied itself that `pulse_count` cannot change, it reasoned that if the
condition is false when you arrive it will be false forever, and emitted an
unconditional jump to itself. Your twenty-slot wait has been compiled into two
bytes of machine code that can never, under any circumstances, terminate.

Your addresses will differ from these. The shape will not.

Paste both loops into your logbook. That is the evidence you are handing in.

> The compiler did not break your code. It read your code, proved that nothing
> inside that loop could possibly change the variable, and acted on the proof.
> The proof was wrong, and it was wrong because you never told the compiler that
> an interrupt exists. **Your job is to work out what keyword you owe it.**

---

## Questions to ask the code

Not hints. Questions. Turn each one into a written hypothesis before you test it.

**On the shared state (3 defects live here)**

- Which variables are written by `encoder_isr` and read by `main`? List them.
  For each one, what stops `main` seeing a half-finished value?
- The Cortex-M0+ is a 32-bit machine. How many instructions does it take to load
  a `uint64_t`? What happens if the interrupt fires between them? Which variable
  in this file is at risk, and what would the symptom look like?
- What is the difference between what `volatile` guarantees and what *atomicity*
  guarantees? Does `volatile` fix the previous question? (It does not. Why not,
  and what does?)

**On the debounce (1 defect)**

- `time_us_32()` returns a 32-bit microsecond counter. How long until it wraps
  back to zero? Work it out — it is not a round number of hours.
- Consider `now > last + DEBOUNCE` versus `now - last > DEBOUNCE`. They look
  equivalent. Take `last = 0xFFFFF000`, `DEBOUNCE = 50000`, and `now = 0x00001000`
  and evaluate both by hand in 32-bit unsigned arithmetic.
- **Make it frequent before you fix it.** Do not wait 71 minutes. Seed the
  variable near the wrap at startup and reproduce the fault in seconds:
  ```c
  last_debounce_us = 0xFFFFF000u;   /* TEMPORARY - reproduce the wrap */
  ```
  This is the single most useful trick in this hunt, and it generalises: *if a
  bug is rare because a counter has to reach a certain value, set the counter.*

**On the state machine (1 defect)**

- Trace `state`, `slot_start_us` and `slot_width_us` by hand for the first three
  edges. Write the table out on paper.
- What does the printed width tell you about how many edges the machine thinks it
  needs to complete one slot? Compare that to how many it actually needs.

**On the ISR itself (2 defects)**

- How long does `printf` take at 115200 baud? Compare that to the width of one
  encoder slot at a realistic wheel speed. What happens to edges that arrive
  during the `printf`?
- Remove the `printf` from the ISR — do not fix anything else — and rerun. Does
  a *different* symptom appear or get worse? Record it. That is not a
  coincidence, and it is the definition of a Heisenbug: **the instrument was
  holding the system together.**
- `DEBOUNCE_US` is 50000. What wheel speed makes a real slot shorter than that?
  Above that speed, what does the driver report, and does it report it as an
  error or as a plausible-looking wrong number? (The second is far worse.)

**On the interrupt configuration (1 defect)**

- The ISR is registered for one edge type. The slot-width calculation assumes
  something about which edges it will see. Are those two assumptions compatible?
- What would `slot_width_us` measure if you only ever see one edge per slot?

---

## The instrument you should be reaching for

Stop printing from the ISR. Do this instead:

```c
#define PROBE_PIN 15

void encoder_isr(uint gpio, uint32_t events) {
    gpio_put(PROBE_PIN, 1);        /* ISR entry  - about 2 CPU cycles */
    ...
    gpio_put(PROBE_PIN, 0);        /* ISR exit                        */
}
```

Now put a scope or a second Pico on GP15. You can see *that* the ISR ran, *when*
it ran, and *how long it took* — for a cost of roughly 30 nanoseconds instead of
`printf`'s several milliseconds.

When you need actual values rather than timing, log them into a ring buffer from
the ISR and print them from `main`:

```c
static volatile uint32_t log_buf[64];
static volatile uint8_t  log_head = 0;
/* in the ISR:  log_buf[log_head++ & 63] = slot_width_us;   */
/* in main:     drain and printf at your leisure            */
```

**Rule to take away: an ISR sets a flag and stores a number. It does not do work,
and it certainly does not do I/O.**

---

## Hint

<details>
<summary><b>Sealed — open only after 30 minutes of real effort on the questions above</b></summary>

The eight defects, by area, so you know where to keep looking:

| Area | How many | Class |
|---|---|---|
| Shared state between ISR and `main` | 2 | 1 Heisenbug, 1 Heisenbug |
| Debounce arithmetic | 1 | logical |
| Debounce *duration* | 1 | logical |
| The `switch` statement | 1 | logical |
| `printf` in the ISR | 1 | Heisenbug |
| Interrupt edge configuration | 1 | logical |
| The final report in `main` | 1 | undefined behaviour |

If you have found six and cannot place the last two: **two of these the compiler
already told you about** (build with the warnings on and read them), and one is
not visible at all until you ask what happens on the *second* revolution of the
wheel at speed.
</details>

---

## Hand in

- `LOGBOOK.md`, at least **eight** defect rows plus your hypothesis trail.
- The two disassembly extracts from Task A, with the differing instruction
  highlighted.
- In the reflection, answer this:

> You removed the `printf` from the ISR and a different symptom appeared. Explain,
> in terms of *timing*, why adding an instrument to a system can hide the very
> fault you were trying to observe — and what that implies about any bug you have
> ever "fixed" by adding a print statement.
