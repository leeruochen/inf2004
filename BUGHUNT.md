# THE BUG HUNTS: A Debugging Method

Every lab in this repository has a **Bug Hunt** attached to it — a small piece of
real firmware with defects already planted in it. Your job is to find them.

This page is the method. Read it once, before Bug Hunt #1, and come back to it
whenever you are stuck. The lab briefs get shorter as you go; this page does not.

---

## Why we do this

You have been taught to *write* code. Almost nobody is taught to *debug* it, and
yet debugging is where most engineering hours actually go. Worse, embedded
debugging is its own discipline: there is no `print` you can trust in an
interrupt, no operating system to catch your mistakes, and a bug that only
appears once an hour is still a bug that will ship.

The algorithms in these hunts are the ones firmware actually contains — bit
manipulation, serialisation, state machines, fixed-point filters, ring buffers,
CRCs. You will meet all of them again. But the algorithm is the excuse. The
**method** is the lesson.

---

## The five rules

**1. Reproduce it before you fix it.**
If you cannot make the fault happen on demand, you cannot know that you fixed
it. "I changed something and it stopped happening" is not evidence — intermittent
bugs stop happening on their own all the time.

**2. One change at a time.**
Change two things, and if the symptom moves you have learned nothing about
either. This is the single most violated rule in the lab, and the single most
expensive.

**3. Predict, then run.**
Before you press run, write down what you expect to see. When the output matches
your prediction, you understood the system. When it doesn't, *that gap is the
bug* — and you found it by being surprised, not by scrolling.

**4. A fix you cannot explain is not a fix.**
If you cannot say **why** the change works, you have moved the bug, not removed
it. Very often you have made it rarer, which is strictly worse than leaving it
alone.

**5. Never delete a symptom.**
Commenting out the `printf` that crashes, widening the array until the corruption
stops, adding a `sleep_ms(10)` until the race goes away — these are not fixes.
They are the bug wearing a hat.

---

## The loop

```
        ┌─────────────────────────────────────────┐
        │                                         │
        ▼                                         │
    OBSERVE  ──►  HYPOTHESISE  ──►  EXPERIMENT  ──┘
   what does      what single       one change,
   it actually    cause would       predicted
   do?            explain ALL       outcome
                  of that?
```

**Observe.** Not "it doesn't work". *Exactly* what happens: which line of output
is wrong, at which input, on which run, in which build. Write the actual
observed value next to the expected value.

**Localise.** Cut the search space in half, then in half again. Comment out the
second half of `main()`. Feed a constant instead of a sensor. Run the algorithm
on your laptop instead of the Pico. You are looking for the *boundary* — the
input, the timing, or the flag at which behaviour changes. **A bug with a
boundary is a bug with an explanation.**

**Hypothesise.** One sentence, in the form *"X is wrong, which would explain Y
and also Z."* If your hypothesis explains only one of three symptoms, it is
probably not the whole story.

**Experiment.** Make one change that would *distinguish* your hypothesis from the
alternatives, and predict the result first.

---

## Your instruments, in order of desperation

| Instrument | Cost | Use it when |
|---|---|---|
| Compiler warnings (`-Wall -Wextra`) | free | Always. Turn them on before you do anything else. |
| Reading the code aloud | free | You have been staring for 10 minutes. |
| `printf` | **high** — changes timing | The bug is not timing-related. |
| Host build (`gcc` on your laptop) | free | The algorithm is pure maths. Fastest possible loop. |
| Golden test vector | cheap | You need to know *whether* it is right, not just plausible. |
| GPIO pin toggle | ~2 cycles | You need to see timing without disturbing it. |
| Deferred logging (ring buffer) | low | You need `printf` detail with `printf` cost paid later. |
| LED blink codes | free | You have no serial output at all. |
| RTOS introspection | low | The bug involves more than one task. |
| Debugger breakpoint | halts the world | The system can survive being stopped. |
| **Data watchpoint** | free, in hardware | You know *what* is corrupted but not *who* did it. |
| Disassembly view | free | The source and the behaviour disagree. |

Note where `printf` sits. From Bug Hunt #3 onward, `printf` is sometimes *part
of the bug* — it is slow enough to hide a race, and it can deadlock inside an
interrupt. Learning when to put it down is most of the point of this exercise.

---

## The four kinds of defect

| Kind | Who catches it | How it feels |
|---|---|---|
| **Syntax** | the compiler | "It won't build." Annoying, never dangerous. |
| **Logical** | your test | "It builds fine and the answer is wrong." |
| **Heisenbug** | nobody, reliably | "It works when I watch it." Adding a probe changes or hides it. |
| **Undefined behaviour** | nobody at all | "It works on my machine / at -O0 / on Tuesdays." |

The hunts move down this table. Hunt #1 is mostly syntax. Hunt #6 is almost
entirely the bottom two rows.

> **On undefined behaviour:** when the C standard says a construct is undefined,
> the compiler is allowed to assume it *never happens* and optimise on that
> assumption. This is why UB bugs so often appear only when optimisation is
> enabled — the compiler didn't break your code, it believed you.

---

## The logbook

Every hunt asks you to keep one. It is the assessed artefact, not the fixed
code — fixed code is trivial to copy, and reasoning is not.

| Symptom observed | Hypothesis | Experiment (one change) | Result | Conclusion |
|---|---|---|---|---|
| Hangs after ~20 edges, Release build only | `main` is not re-reading `pulse_count` | Rebuild at `-O0`, no source change | Completes normally | Optimiser-visible → missing `volatile`; confirmed in disassembly |

One row per hypothesis, **including the wrong ones**. The wrong hypotheses are
what proves you were doing this properly rather than guessing until it compiled.

---

## Rules of the hunt

- Each brief tells you **how many defects are planted**. Trust the number — if
  you have found six of seven, keep going; if you think you have found nine of
  seven, one of your "fixes" broke something.
- The defect count includes compiler warnings. `-Wall -Wextra` is not optional.
- You may not consult another group's fixed source. You may absolutely argue
  with them about a hypothesis.
- Getting stuck for twenty minutes is normal and is where the learning is. Getting
  stuck for two hours is not — ask.

---

## The ladder

| # | Lab | Algorithm | Defects | New instrument |
|---|---|---|---|---|
| [1](LAB1/bughunt/) | Environment | Bit counting & manipulation | 6 | Reading a compiler error that points at the wrong line |
| [2](LAB2/bughunt/) | GPIO & UART | Serialisation & framing | 7 | Hex-dumping both ends and diffing |
| [3](LAB3/bughunt/) | Interrupts & timers | Edge state machine, debounce | 8 | GPIO probe, deferred logging, `-O0` vs `-O2` |
| [4](LAB4/bughunt/) | PWM & ADC | Fixed-point IIR filter | 8 | Host unit test with synthetic input |
| [5](LAB5/bughunt/) | FreeRTOS | Ring buffer & moving average | 9 | Stack high-water marks, `configASSERT` |
| [6](LAB6/bughunt/) | Optimisation & debugging | CRC-8, LFSR, frame parser | 12 | Watchpoints, disassembly, HardFault decode |

Guidance decreases deliberately. Hunt #1 walks you through the first defect and
gives you a hint for every remaining one. Hunt #6 gives you a specification, a
number, and nothing else.

That is not us being unhelpful. That is the exercise.
