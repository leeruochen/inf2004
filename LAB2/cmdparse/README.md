# CHALLENGE — The command that arrives as text

> **Do Bug Hunt #2 first.** This is its mirror image, and the contrast is the
> point.

Bug Hunt #2 puts a sensor reading on the wire as **bytes**: a start byte, a
length, nine bytes of big-endian payload, a checksum. Compact, fast, and
completely unreadable by a human with a serial terminal.

This puts commands on the same wire as **text**:

```
PING\r\n
GET TEMP\r\n
SET LED 2 ON\r\n
```

Readable by anyone, debuggable with nothing but a terminal, and — as you are
about to find out — considerably harder to parse correctly than the binary
format was.

Almost every device you will ever talk to uses one of these two shapes. Modems
take AT commands. GPS receivers emit NMEA sentences. Test equipment speaks SCPI.
All of them are lines of ASCII arriving one character at a time down a wire,
from a sender who may be badly behaved, and all of them have to be parsed by a
microcontroller with no operating system underneath it to catch mistakes.

---

## The specification

At the top of [`cmdparse.h`](cmdparse.h). Read it before you write anything.
It is correct — unlike the code, which does nothing yet.

Three rules are worth repeating here, because they are what makes this an
*embedded* exercise rather than a first-year C exercise:

**No `malloc`.** Not one byte. You have 264 KB of SRAM, no virtual memory, and
no way to recover from a leak in a device that is expected to run for months.
Everything you need is either on the stack or in the caller's buffer.

**No `strtok()`.** It works, and it is a trap. `strtok` keeps its position in a
hidden `static` variable, so two links parsing at once, or one parse interrupted
by another, corrupts both. Bug Hunt #5 is nine defects of exactly this shape.
Split the line yourself; it is about eight lines of code.

**Tokenise in place.** Write NULs over the separators in the buffer you were
given and hand back pointers into it. Do not copy the words somewhere else.
This is why the parser takes a writable `char *`, and the test harness checks
that you did it — `argv[0]` must literally equal `line`.

---

## Work on your laptop first

```bash
gcc -Wall -Wextra -o cmdparse_host cmdparse_host.c cmdparse.c
./cmdparse_host
```

Sixty-odd assertions, all hand-derived from the specification, and they run in
about a millisecond. **Get to `ALL TESTS PASS` before you plug anything in.**

This is the instrument from row 4 of the table in [`BUGHUNT.md`](../../BUGHUNT.md):
*host build — free — the algorithm is pure logic, fastest possible loop.* A
parser is pure logic. There is nothing about `cmd_split()` that requires a
microcontroller, and everything about debugging it on one that will slow you
down.

Do not edit the vectors to make your code pass. It is a cover-up rather than a
fix, it is Rule 5 of the method, and it is obvious in marking.

---

## Then the hardware

Same wiring as Bug Hunt #2 — `GP8`/`GP9` crossed to your partner's board, common
ground — plus four LEDs on `GP2`–`GP5`. Build with the `CMakeLists.txt` here and
type commands at your partner's terminal.

[`cmdparse_pico.c`](cmdparse_pico.c) is written for you and needs no changes.
Note where its `printf` goes: over **USB**, never over the link. If your replies
and your debug output share a UART you will lose characters and spend the
afternoon debugging the debugger — which is precisely the fault described in
*"A Deliberate Intermittent Fault"* in the lab README.

---

## Five functions, in the order worth writing them

| # | Function | The part that catches people |
|---|---|---|
| 1 | `cmd_word_eq` | **Done for you** — read the comment above it. It explains why `toupper()` from `<ctype.h>` is undefined behaviour on a plain `char` here. |
| 2 | `cmd_parse_u8` | Where you check for overflow. `"99999999999"` must be rejected, not wrapped. |
| 3 | `cmd_split` | Leading, trailing and repeated spaces. Returning `-1` rather than writing past `argv[max_args]`. |
| 4 | `cmd_reader_feed` | The `\r\n` terminator, accepting *exactly* `CMD_MAX_LINE` characters, and recovering after an overlong line. |
| 5 | `cmd_parse` | Checking the argument **count** before you touch any argument. |

Number 4 is the hard one, and it is hard for an interesting reason: it is a
**state machine**, which is the whole subject of Lab 3. A reader that has seen 70
characters of an over-long line cannot simply stop storing them — it has to keep
consuming until the terminator arrives, report the error exactly once, and come
back clean. Write down what your state means before you write the code.

---

## Hand in

- `cmdparse.c`, building clean under `-Wall -Wextra` and passing all host tests.
- A short note on `cmd_reader_feed`: what states your reader has, and what
  happens in each on `'\r'`, on `'\n'`, on an ordinary character, and on the
  character after the buffer is already full.
- One paragraph comparing this format with the binary one from Bug Hunt #2:
  which was harder to write, which is harder to debug at three in the morning
  with only a terminal, and which you would choose for a battery-powered sensor
  sending one reading a minute for two years. There is a defensible answer for
  each; there is no defensible answer that ignores the trade-off.
