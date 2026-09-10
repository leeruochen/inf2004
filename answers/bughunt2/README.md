# Answer 2 — serialization and UART

[Exercise](../../LAB2/bughunt/README.md) · [Codec](frame.c) · [Pico link](bughunt2_pico.c) · [Build instructions](../README.md)

| Defect | Observation | Correction |
|---|---|---|
| Struct padding | The length is 12 and unwanted padding bytes appear. | Encode the nine specified payload bytes explicitly. |
| Host byte order | Multi-byte values appear reversed. | Encode each field most-significant byte first. |
| Short copy loop | The length promises one more payload byte than is written. | Emit all nine bytes and sum exactly those bytes. |
| Signed integer promotion | UBSan flags the high timestamp byte shifted by 24. | Convert to `uint32_t` before shifting. |
| Unchecked received length | Truncated frames read past the input; oversized payloads overwrite the stack. | Require payload length 9 and frame length 12 before copying/indexing. |
| Fixed receive windows | A dropped or added byte shifts subsequent frames. | Hunt for the start byte, validate length, collect and check a frame, then resynchronise after rejection. |
| Text translation on binary UART | `0A` becomes `0D 0A` after the transmit hex dump. | Disable CRLF translation and send with `uart_putc_raw`. |

The length mismatch and padding bytes share a struct-layout cause; reversed
fields are the separate endianness issue. Explicit serialization addresses both.
A `sizeof(reading_t) == 12` assertion would not prove wire-format correctness.
The wire spec is independent of the in-memory struct's size.

All three golden frames are correct. Vector A ends in `72`, the low byte of
the payload sum. Vector B exercises a negative temperature and the timestamp's
high bit. The host harness also rejects wrong headers/checksums and malformed
lengths once the valid-frame checks pass. Run with ASan and UBSan: a success
message alone cannot prove every possible input is handled safely.

The reference receiver ignores noise before a start, preserves a possible start
inside a rejected frame, and expires incomplete frames after a 100 ms gap.
`0xAA` may also occur in payload data; finding it is only a candidate boundary,
not proof of a valid frame. A checksum can miss some corruptions.

On two boards, check both directions, consecutive button presses, a reset of
either board mid-frame, and injected junk/truncated frames followed by clean
traffic. Corrected frames containing `0A` must arrive unchanged. These are
hardware checks; the host codec tests do not exercise the UART receiver.
