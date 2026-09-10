# Answer 6 — telemetry and debugging

[Exercise](../../LAB6/bughunt/README.md) · [Corrected source](bughunt6.c) · [Build instructions](../README.md)

| Defect | Why it fails | Correction |
|---|---|---|
| CRC table has 255 entries | Initialization writes entry 255 outside the array. | Allocate 256 entries. |
| Uninitialised CRC accumulator | The initial CRC value is indeterminate. | Start at zero. |
| Unchecked table readiness | Calls made before initialization use a zero-filled table. | Initialize lazily on first CRC call. |
| Incorrect LFSR mask | `0xB000` does not generate the required maximal period. | Use the right-shifting Galois mask `0xB400`. |
| Period test stops at 200 | Even a correct generator cannot complete its cycle. | Permit 65535 transitions; retain a safety bound. |
| Cast-based timestamp load | The pointer can be unaligned and the native byte order is wrong. | Assemble the four big-endian bytes explicitly. |
| Incomplete frame validation | The advertised length can exceed the input or omit required fields. | Require payload length 6 and total length 9 before CRC or field access. |
| Returning local storage | The returned pointer outlives the local array. | This reference uses static storage; a caller-owned buffer is preferable for concurrent callers. |
| Missing string terminator space | `strcpy` writes strlen+1 bytes. | Allocate that size and check malloc. The caller frees the result. |
| Empty delay loop | Optimization can remove it; iteration count is not a timing guarantee. | Use `sleep_us(500)` on Pico, a timed sleep on the host. |
| Non-volatile completion flag | The wait may never reload an ISR update. | Use volatile for this single-core ISR flag. |
| Eight-bit checksum index | The index wraps before reaching length 300. | Use a 16-bit index matching the length type. |

For the watchpoint exercise, use the broken Debug build. Break at `crc_init`,
then watch the first byte outside the table:

```text
break crc_init
continue
watch -l *(unsigned char *)((char *)&crc_table + sizeof crc_table)
continue
```

The debugger should stop when initialization writes index 255. Watch the address,
not an assumed neighbour: static declarations do not guarantee adjacent storage,
and the initialized LFSR state is typically in a different section. In the fixed
build initialization never writes past the 256-byte array.

For the HardFault, inspect the broken parser's word load and actual input address.
On Cortex-M0+, an unaligned word load faults. If earlier UB rejects the frame or
changes execution, first fix the CRC issues and repeat. Do not infer a guaranteed
crash solely from C declaration order or the optimization setting.

The Pico harness schedules an alarm ISR and calls `wait_for_conversion`, so its
wait loop remains available for disassembly. Compare both that function and
`calibration_delay` in Debug and Release. The fixed flag must be reloaded; the
fixed delay must still invoke timed waiting.

The host harness verifies lazy CRC initialization, the CRC vector, LFSR period, valid and malformed frames,
string results and 300-byte checksum. Passing it does not validate target timing
or the live ISR. Returning a local buffer is invalid at every optimization level;
different builds need not produce different symptoms. The separate `pid.c`
exercise remains an optional follow-on, outside these twelve fixes.
