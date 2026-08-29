# BUG HUNT #1 — Bits that lie about themselves

> **Guidance level: maximum.** This brief walks you through the first defect from
> start to finish, then gives you a hint for every remaining one. Later hunts
> will give you much less. Read [`../../BUGHUNT.md`](../../BUGHUNT.md) first — it
> is the method you will use for all six.

| | |
|---|---|
| **Algorithm** | Bit counting, parity, bit reversal, mask testing |
| **Defects planted** | **6** — 3 syntax, 2 logical, 1 undefined behaviour |
| **Runs on** | Your laptop *and* the Pico. Start on the laptop. |
| **Time** | ~45 minutes |
| **You must hand in** | `LOGBOOK.md` |

---

## The situation

`bughunt1.c` is a small library of bit helpers — the kind of code that sits at
the bottom of every GPIO driver. It ships with a test harness that knows the
right answers.

**Correct behaviour:** every line prints `ok`, and the program ends with
`ALL TESTS PASS  (0 failures)`. The exact output you are aiming for is in
[`expected.txt`](expected.txt).

**Current behaviour:** it does not compile.

---

## Step 1 — Build it and read the errors

You do **not** need the Pico for this. Open a terminal in this folder:

```bash
gcc -Wall -Wextra -o bughunt1 bughunt1.c
```

You will get a wall of errors. This is normal and it is not six errors — it is
one or two real problems producing a cascade. **Always fix the first error and
rebuild.** Never try to fix the whole list at once.

---

## Step 2 — A worked example (defect 1 of 6, free of charge)

Here is the first error GCC gives:

```
bughunt1.c:28:1: error: unknown type name 'uint8_t'
   28 | uint8_t count_bits(int value)
      | ^~~~~~~
bughunt1.c:16:1: note: 'uint8_t' is defined in header '<stdint.h>';
                       did you forget to '#include <stdint.h>'?
```

**Observe.** The compiler does not know what `uint8_t` is.

**Hypothesise.** `uint8_t` is not a built-in C type. It comes from a header. The
note tells us which one.

**Experiment.** Add `#include <stdint.h>` next to the other includes at the top
of the file. Rebuild.

**Result.** Roughly two-thirds of the errors vanish, because every one of them
was the same missing header reported at a different line.

> **Lesson:** the number of errors tells you almost nothing about the number of
> defects. One missing header can produce forty errors.

Now rebuild and look at what is left:

```
bughunt1.c:32:5: error: expected ',' or ';' before 'while'
   32 |     while (value) {
      |     ^~~~~
```

Read this one carefully, because it teaches the single most important thing
about compiler errors:

```c
30 |     uint8_t count = 0        <-- the defect is HERE
31 |
32 |     while (value) {          <-- the error is reported HERE
```

The compiler was still reading the statement that began on line 30 when it hit
`while` on line 32 and gave up. **A compiler error tells you where the compiler
stopped coping, not where you made the mistake.** When the reported line looks
fine, look *upward*.

That is defect 1 (missing `<stdint.h>`) and defect 2 (missing semicolon). Record
both in your logbook. Four to go.

---

## Step 3 — Finish the build

There is **one more syntax defect**. Same technique: fix the first error, rebuild,
read again. When the file compiles, move to Step 4.

---

## Step 4 — Now the hard part

It compiles. It is still wrong. Run it:

```bash
./bughunt1
```

Some tests print `FAIL`, and one of them does something worse than fail. Work
through them one at a time, top to bottom.

For each failure, before you change anything, write in your logbook:

- what the test asked for
- what it actually returned
- **what a single defect would have to be, to produce exactly that wrong value**

That last question is the whole skill. `got 0` when you expected `1` is a very
different clue from `got 0x1E6A2C48` when you expected `0x1E6A2C49`.

---

## Hints

Open one only after you have spent ten minutes on your own. There is one hint
per remaining defect, in the order you will meet them.

<details>
<summary><b>Hint — the program stops and never finishes</b></summary>

It is not crashed. It is spinning.

Which test is it stuck on? Look at the last line it printed, then look at the
*next* test in `main()`. That is the call that never returned.

Now look at the loop inside that function. The loop ends when `value` becomes
zero. Ask yourself: for the specific argument being passed, **does `value` ever
actually reach zero?**

Pay attention to the *type* of `value`, and to what `>>` does to a negative
number. The C standard calls this an *arithmetic shift*: it copies the sign bit
in from the left, forever.
</details>

<details>
<summary><b>Hint — <code>pin_is_clear</code> gives the same answer for every pin</b></summary>

The function returns the same result no matter which pin number you pass, which
means the pin number is not actually reaching the comparison.

C's operator precedence is not left-to-right. `==` binds **tighter** than `&`.
Put brackets around what you *meant* and compare the two readings:

```c
(mask & (1u << pin)) == 0      /* what you meant  */
mask & ((1u << pin) == 0)      /* what you wrote  */
```

The second one computes `mask & 0` or `mask & 1` — the shift result is thrown
away entirely.

**Take this away:** when a bit-manipulation expression mixes `&`, `<<` and `==`,
put brackets in even when you are sure. You will be wrong about the precedence
more often than you think.
</details>

<details>
<summary><b>Hint — <code>reverse_bits</code> is close but not right</b></summary>

Compare the value you got to the value expected, **in binary**, not hex. Add a
temporary line to print both with `%032b`-style formatting (C has no `%b`, so
write a tiny loop, or convert by hand for one case).

Then count the iterations of the loop. How many bits are there in a `uint32_t`?
How many times does `for (int i = 0; i <= 32; i++)` run?

On the very last iteration, work out by hand what `v >> i` and `1 << (31 - i)`
evaluate to. One of those two is asking the CPU to do something the C standard
explicitly refuses to define.

> This is the defect class that will hunt you for the rest of the module.
> A shift by an amount greater than or equal to the width of the type is
> **undefined behaviour**. It might give 0. It might give `v`. It might give
> whatever was in the register. It may behave differently on your laptop and on
> the Pico — and it is allowed to.
</details>

<details>
<summary><b>Hint — is <code>swap_nibbles</code> broken at all?</b></summary>

No. It is correct.

Not every function in a broken file is broken, and one of the most expensive
debugging mistakes is "fixing" working code because you are already in the mood
to change things. If your logbook has a row where you modified `swap_nibbles`,
that row is a lesson too — write down what made you suspect it.
</details>

---

## Step 5 — Run it on the actual hardware

Once all tests pass on your laptop, build it for the Pico:

```bash
mkdir build && cd build
cmake -DPICO_BOARD=pico_w ..
make
```

Copy `bughunt1.uf2` onto the Pico and open the serial monitor. You should see
the identical output.

**Why bother, if it already passes on the laptop?** Because "it works on my
machine" is the oldest lie in software. The laptop is a 64-bit x86 with a
different compiler, different type sizes and different default signedness. From
Bug Hunt #2 onward, that difference will bite you deliberately.

---

## Hand in

`LOGBOOK.md` in this folder, with **at least six rows** — one per defect — plus
any wrong hypotheses you tried along the way. The wrong ones count. They are the
evidence that you were reasoning rather than guessing.
