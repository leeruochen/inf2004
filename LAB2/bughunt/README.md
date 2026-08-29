# BUG HUNT #2 — The frame that arrives wrong

> **Guidance level: high, but stepping down.** You still get a worked technique
> and hints, but the hints are now grouped by *area* rather than one per defect —
> there are fewer hints than there are defects. Method: [`../../BUGHUNT.md`](../../BUGHUNT.md).

| | |
|---|---|
| **Algorithm** | Serialisation, framing, checksums |
| **Defects planted** | **7** — 5 in `frame.c`, 2 in `bughunt2_pico.c` |
| **Runs on** | Laptop finds 5 of 7. The other 2 need two real Picos. |
| **Time** | ~60 minutes |
| **You must hand in** | `LOGBOOK.md` |

---

## The situation

A sensor reading has to travel from one Pico to another over UART. To do that it
must be flattened into a stream of bytes, framed so the receiver knows where it
starts and ends, checksummed so corruption is detected, and rebuilt on the far
side.

**The specification is at the top of [`frame.h`](frame.h). The specification is
correct. The code is not.**

That distinction matters more than it sounds. In real firmware the wire format is
agreed with another team, another company, or a chip you cannot modify. You do
not get to redefine the protocol because your encoder is easier to write that
way. **Fix the code to match the spec, never the spec to match the code.**

---

## Step 1 — Look at the bytes

```bash
gcc -Wall -Wextra -o bughunt2_host bughunt2_host.c frame.c
./bughunt2_host
```

The harness encodes three known readings, prints what the specification says the
bytes should be, prints what your encoder actually produced, and diffs them.

The very first line of output is a gift:

```
sizeof(reading_t) = 12 bytes
```

The specification says the payload is **9 bytes**. Sit with that for a moment
before reading on. Nothing in `reading_t` is bigger than it looks — 2 + 1 + 2 + 4
is nine. So where did the other three bytes come from, and *where inside the
struct are they*?

> **The technique this week is: hex-dump at every boundary.** Not the decoded
> value — the raw bytes, on both sides of the wire, and diff them by eye. A
> decoded value tells you *that* something is wrong. The bytes tell you *what*.

---

## Step 2 — Read the diff like a forensic scientist

Here is real output from the broken encoder for vector A:

```
expected   [12] AA 09 12 34 01 00 FD 0A 0B 0C 0D 72
encoded    [14] AA 0C 34 12 01 00 FD 00 00 00 0D 0C 0B 68
```

Do not fix anything yet. Extract every independent fact you can:

- The length byte says `0C`, not `09`.
- `12 34` came out as `34 12`. So did `0D 0C 0B` versus `0A 0B 0C 0D`.
- There are `00 00 00` bytes in the middle that the specification never asked for.
- The frame is 14 bytes, but the length byte claims 12 payload bytes — 2 header
  + 12 payload + 1 checksum would be 15. **The frame is internally inconsistent
  with its own length field.**

That is four distinct clues, and they are **not** four separate defects — some of
them share a cause. Working out which symptoms collapse into one root cause is
the actual exercise. Write all four observations in your logbook as separate
rows, then start merging them as you form hypotheses.

---

## Step 3 — A new instrument: the undefined-behaviour sanitiser

Your laptop compiler can be asked to check, at runtime, for operations that C
leaves undefined:

```bash
gcc -Wall -Wextra -fsanitize=undefined -o bughunt2_ub bughunt2_host.c frame.c
./bughunt2_ub
```

Try it. It will name a file, a line, and the exact operation:

```
frame.c:48:36: runtime error: left shift of 128 by 24 places
               cannot be represented in type 'int'
```

**This is a defect you would almost certainly not have found by reading**, and it
is one of the nastiest kinds: the code produces the right answer today, on this
compiler, at this optimisation level. Work out *why* shifting a `uint8_t` left by
24 involves a type called `int` at all — the answer is "integer promotion", and
it is responsible for an enormous share of real firmware bugs.

You cannot run the sanitiser on the Pico. This is one more reason to do as much
work as possible on the laptop first.

---

## Step 4 — Onto the hardware

When the host harness prints `CODEC MATCHES THE SPECIFICATION`, flash
`bughunt2_pico.c` to **both** boards, wire them up as described in the file
header, and press GP20 on one of them.

Two defects remain, and they only exist on a real link. Expect the receiver to
print something like:

```
tx         [12] AA 09 12 34 01 00 FD 0A 0B 0C 0D 72
rx         [12] AA 09 12 34 01 00 FD 0D 0A 0B 0C 0D
           BAD FRAME (checksum or header rejected)
```

and then for **every subsequent frame to be wrong too, forever**, even though you
have not changed anything.

Those are two separate defects: one puts a byte on the wire that should not be
there, and one means the receiver can never recover once it is out of step. Both
are extremely common in real products.

---

## Hints

One hint per *area*, not per defect. Some areas contain more than one defect.

<details>
<summary><b>Hint — the encoder (3 defects here)</b></summary>

The encoder takes a `reading_t *`, casts it to `uint8_t *`, and copies raw bytes
off the front of the struct. Ask two questions about that approach:

1. **Is the struct laid out the way you assume?** The compiler is allowed to
   insert unnamed padding between members so each one lands on an address it can
   access efficiently. Print `offsetof(reading_t, temp_c_x10)` and
   `offsetof(reading_t, timestamp_ms)` (from `<stddef.h>`) and compare them to
   the offsets the specification demands.

2. **Is the byte order the way you assume?** The RP2040 and your laptop are both
   little-endian; the specification is big-endian. A raw memory copy preserves
   the machine's order, not the protocol's.

The fix for both is the same, and it is the fix used in essentially all real
protocol code: **do not copy structs onto a wire.** Write each field out
explicitly, one byte at a time, most-significant byte first. It is more typing
and it is correct on every compiler and every architecture.

There is one further defect in the encoder that has nothing to do with layout.
Compare the number of payload bytes the loop writes against the number the length
byte promises. Then check whether the checksum covers exactly the bytes the
specification says it covers.

**Make the fix permanent.** Add this next to the struct so nobody can reintroduce
the padding defect:
```c
#include <assert.h>
static_assert(sizeof(reading_t) == 12, "layout changed - check frame_encode");
```
</details>

<details>
<summary><b>Hint — the decoder (2 defects here)</b></summary>

One is the sanitiser finding from Step 3. Fixing it is a matter of making the
promotion explicit rather than accidental — cast to the type you actually want
*before* you shift, not after you have already lost the value.

The other is a security defect, and the harness will not find it because the
harness only ever sends well-formed frames. Look at where the length byte comes
from, and where it is used:

```c
len = in[1];                              /* attacker-controlled */
for (uint8_t i = 0; i < len; i++)
    payload[i] = in[2 + i];               /* how big is payload[]? */
```

`len` arrives over a wire. It could be anything — corruption, noise, or a
deliberately hostile device. `payload` is a fixed-size local array, which means
it lives on the stack, next to your return address.

**Never trust a length that came from outside your program.** Validate it against
both your buffer size and the number of bytes you actually received, before you
use it as a loop bound. Try feeding the decoder a frame with `len = 200` and see
what happens.
</details>

<details>
<summary><b>Hint — the link (2 defects here) — open only after you have two boards running</b></summary>

**On the extra byte.** Compare the `tx` and `rx` hex dumps byte by byte. If the
receiver saw a byte the transmitter never printed, the byte was added *after*
`hexdump()` ran — so it was added by the transmit path itself, not by the
encoder. Look at every line of `link_init()` and ask what each one does to a
byte before it reaches the wire. One of them is documented as a convenience for
text. This payload is not text. Which byte value would that convenience react
to, and is that value present in the frame? (Check the hex dump.)

> This is a genuinely nasty class of bug: a helper that is correct for the use
> case it was designed for (printing strings) and silently corrupting for the
> use case you put it to (moving binary data). The SDK offers a `_raw` variant
> of that function for exactly this reason.

**On never recovering.** The receiver counts bytes and declares a frame complete
when it has 12 of them. It never checks that byte 0 is actually `0xAA`. So a
single spurious or dropped byte shifts the window by one — permanently. Every
frame from then on is a mixture of two real frames.

A real receiver is a small state machine: *hunt for `0xAA`* → *read the length* →
*read that many payload bytes* → *check the checksum* → back to hunting. If any
step fails, it goes back to hunting rather than blindly continuing. You will
build exactly this state machine properly in Bug Hunt #3.
</details>

---

## Something worth knowing before Bug Hunt #3

Plain `char` is **unsigned** on the Pico's compiler (`arm-none-eabi-gcc`) and
**signed** on your laptop's x86 GCC. If you ever store a received byte in a
`char` and compare it to `0xAA`, that comparison is true on the Pico and false on
your laptop, from identical source.

Nothing in this hunt depends on it. But when the host harness and the hardware
disagree and you cannot see why, this is the first thing to check — and it is why
"it passed on my laptop" is never the end of the argument.

Use `uint8_t` for bytes. Always.

---

## Hand in

`LOGBOOK.md`, at least **seven** defect rows plus your hypothesis trail. In the
reflection section, answer this one specifically:

> Two of the four symptoms you listed in Step 2 had the same root cause. Which
> two, and what made you realise they were not independent?
