# EXERCISE — Four things that are slower than they need to be

The old version of this exercise said, in full:

```c
// Apply optimizations (you can add your optimizations here)
```

This is that sentence, made specific.

---

## What you are doing

Four functions come in pairs. The `_slow()` version is written for you and
**works** — it is not buggy, it is merely expensive. You write the `_fast()`
version.

| # | Function | The thing to notice |
|---|---|---|
| 1 | `mv_convert` | ADC counts → millivolts, in floating point, on a core with no FPU |
| 2 | `count_in_band` | An expensive call whose arguments never change, inside the loop |
| 3 | `crc8` | Eight iterations per byte, when there is a way to do one |
| 4 | `normalise` | `n` divisions by the same number, on a core with no divide instruction |

Read the specification at the top of [`optimise.h`](optimise.h) before you start.

---

## The rules

**1. The answer may not change.** `_fast()` must return exactly what `_slow()`
returns, element by element, for every input the harness tries. The harness
checks correctness **before** it prints a single timing, and refuses to time
anything that fails. An optimisation that changes the answer is not an
optimisation, it is a defect that happens to run quickly.

**2. You may not touch the `_slow()` versions, the harness, or the test data.**
Making the benchmark easier is not making the code faster.

**3. Report three things per function**, not one:

- **microseconds before and after, measured on the Pico** — not on your laptop;
- **why**, in one sentence naming the actual mechanism;
- and for at least one of the four, a **disassembly extract** proving it.

> A number without a mechanism is a measurement, not an explanation.
> A mechanism without a number is a belief.

---

## Why this processor makes these choices matter

The RP2040 has two Cortex-M0+ cores, and two facts about that core drive three
of these four exercises. Neither is true of the laptop you have been testing on
all semester:

- **There is no floating-point unit.** Every `float` and `double` operation is a
  call into a software library — tens to hundreds of cycles for something that
  costs one cycle on your machine.
- **There is no hardware divide instruction.** ARMv6-M has no `SDIV` or `UDIV`.
  Every `/` and `%` the compiler cannot fold into a shift becomes a call to
  `__aeabi_uidiv`. (The RP2040 does have a hardware divider in its SIO block and
  the SDK will use it — but a division is still far more expensive than a
  multiply.)

This is why *"it was fast enough on my machine"* is not evidence.

---

## Order of work

**1. Correctness, on your laptop.**

```bash
gcc -O2 -Wall -Wextra -o optimise_host optimise_host.c optimise.c
./optimise_host
```

The stubs as shipped call the slow versions, so they start out correct and
exactly as fast — every row says `no change`. That is your baseline.

Two of these are much harder to get *exactly* right than to get *approximately*
right, and the harness is unforgiving on purpose. In particular: a 16.16
fixed-point reciprocal in `normalise_fast` is **not** precise enough. It
disagrees with the slow version on twelve elements. Finding out that your clever
version is subtly wrong is the single most valuable thing this exercise does for
you, because in real work nobody hands you a harness.

**2. Timing, on the Pico.** Build with the `CMakeLists.txt` here and run
[`optimise_pico.c`](optimise_pico.c), which is complete and which you should not
modify. It is the instrument, and an instrument you have adjusted until it gave
the reading you wanted is not an instrument.

**3. Three optimisation levels.** Build at `-O0`, `-O2` and `-O3` and report all
three. Some of your hand optimisations will turn out to be things the compiler
was already doing at `-O2`. **Finding out which ones is most of the exercise** —
an "optimisation" that only helps at `-O0` is not an optimisation, it is you
doing unpaid work on the compiler's behalf.

```bash
arm-none-eabi-objdump -d build-O2/optimise.elf > O2.asm
```

---

## A question you must answer for #3

The table-driven CRC has a **price**, and it is not paid in time. Say what it
costs, in bytes, and name a device on which you would refuse to pay it. "It is
faster" is half an answer; every optimisation in this list is a trade, and an
engineer who can only see the side of the trade they like is the one who fills
your flash and then wonders where it went.

---

## Hand in

- `optimise.c`, passing every correctness check, building clean under
  `-Wall -Wextra`.
- A table: four functions × three optimisation levels × before/after
  microseconds, measured on the Pico.
- One sentence per function naming the mechanism.
- One annotated disassembly extract.
- Your answer to the CRC trade-off question above.
- **One row for anything that did not get faster**, with your explanation of
  why. Compare your `-O0` numbers against your `-O2` ones before you claim
  credit: where the gap closes as the optimisation level rises, the compiler was
  already doing what you did by hand. Reporting that honestly is worth more than
  a speedup, and claiming a speedup the numbers do not show is the one thing
  here that cannot be recovered from.
