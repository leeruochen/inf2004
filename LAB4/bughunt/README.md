# BUG HUNT #4 — The maths is right on paper

> **Guidance level: low.** You get the specification, the symptom, and a
> technique you have not used yet. No hints, no questions list, no sealed
> envelope. You already own the method — [`../../BUGHUNT.md`](../../BUGHUNT.md) —
> and from here on it is yours to apply.

| | |
|---|---|
| **Algorithm** | Fixed-point scaling and a first-order IIR filter |
| **Defects planted** | **8** — 5 in `algo.c`, 3 in `bughunt4.c` |
| **Runs on** | Laptop finds 5 in one second. The other 3 need the Pico. |
| **Time** | 60–75 minutes |
| **You must hand in** | `LOGBOOK.md` and one plotted step response |

---

## The situation

A voltage arrives at the ADC. It is scaled to millivolts, smoothed by a filter,
and mapped onto a PWM duty cycle. Four small functions, none longer than four
lines, every one of them arithmetically defensible if you read it quickly.

The specification is the comment block at the top of [`algo.h`](algo.h). It is
correct.

```bash
gcc -Wall -Wextra -o bughunt4_host bughunt4_host.c algo.c
./bughunt4_host
```

Read the compiler warnings. Then read the failures. Then read the **step
response**, which is the part of the output that matters most.

---

## The technique this week: synthetic inputs

You have been debugging against real sensors. Real sensors are terrible for
debugging: the input is noisy, unrepeatable, and you do not independently know
what the right answer was.

Instead, feed the algorithm a signal whose correct output you can work out on
paper. A **step** is the best one:

```
input:    0 0 0 0 3000 3000 3000 3000 3000 3000 ...
```

A first-order filter with α = 1/16 fed a step must rise smoothly and settle on
the target. That is not an opinion — it is what the maths says, and you can
sketch the curve before you run anything. Then feed it a step *downward* and
demand the mirror image.

The harness already does this. Look at what comes out:

```
step response - rising 0 -> 3000
     187   1087   1699   2115   2396   2588   2718   2805   2865   2906
  (should climb smoothly and settle near 3000)

step response - falling 3000 -> 500
  268438233 1561219340 2438932590 3034842096 3439425414 ...
  (should fall smoothly and settle near 500)
```

Rising: textbook. Falling: the filter leaves the solar system.

**A bug that only appears in one direction is telling you something very
specific.** Work out what is different about the falling case, arithmetically,
and you have the defect. Do the subtraction by hand in 32-bit unsigned, on
paper, for `x = 500` and `y = 3000`.

For your hand-in, plot both step responses (a spreadsheet is fine) before and
after your fix. A picture of a filter that cannot come down is worth more than a
paragraph describing one.

---

## Then: the hardware

When the host harness prints `SIGNAL CHAIN MATCHES THE SPECIFICATION`, wire GP0
to GP26 and flash `bughunt4.c`.

Three defects remain. What you should expect if you look carefully:

- One is in the ADC sequence, and it makes every reading correct-looking but
  attached to the wrong thing. It is invisible on a steady DC input and obvious
  the moment the input moves. **Design an experiment that makes the input move
  in a way you control.**
- One is in a `printf`, and it prints a number that has no relationship
  whatsoever to the value it claims to show. You have seen this defect class
  before — it is also sitting in `LAB6/pid.c`, waiting for you.
- One is a comparison that can never be true. The compiler warns about it. If
  you did not see the warning, you are not building with `-Wall -Wextra`, and
  that is itself worth a row in your logbook.

And one thing the harness cannot check for you: **measure the actual frequency
of the PWM on GP0.** Do not trust the `#define`. Use the lab's own ADC sampling
exercise, or a second Pico, or a scope. It is close to 20 Hz. It is not 20 Hz.
Work out from the RP2040 datasheet how many counter ticks a PWM period actually
takes, and compare that with what `pwm_wrap_for_hz()` returns.

> **The lesson underneath all of this:** none of these eight defects is a typo.
> Every one is a piece of arithmetic that a competent person wrote while thinking
> about the maths and not about the *types*. In C, the types are the maths.

---

## Hand in

- `LOGBOOK.md`, at least **eight** defect rows plus your hypothesis trail.
- A plot of the rising and falling step response, before and after.
- In the reflection, answer this:

> Three of these defects (the 16-bit intermediate, the truncating divide, and the
> unsigned subtraction) share a single underlying cause. Name it in one sentence,
> and describe a habit — not a fix, a *habit* — that would have prevented all
> three.
