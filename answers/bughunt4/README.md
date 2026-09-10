# Answer 4 — arithmetic and the signal chain

[Exercise](../../LAB4/bughunt/README.md) · [Arithmetic](algo.c) · [Pico program](bughunt4.c) · [Build instructions](../README.md)

| Defect | Why it fails | Correction |
|---|---|---|
| Narrow millivolt intermediate | `4095 * 3300` cannot fit in 16 bits. | Retain the product in `uint32_t`. |
| Divide before multiplying | `raw / 4095` is zero for nearly all inputs. | Multiply in 32 bits, then divide. |
| Unsigned filter difference | A downward step wraps to a huge positive number. | Subtract signed ADC counts. |
| Shared channel state | Alternating channels changes the same accumulator. | One state per channel; reset all of them. |
| PWM wrap off by one | A counter from zero to wrap takes wrap+1 ticks. | Subtract one from the calculated count. |
| ADC selection order | A is read before selecting channel A. | Select each input before reading it. |
| Float printed with `%d` | Variadic arguments promote float to double, not int. | Use `%f` or print integer millivolts. |
| Unsigned control error | `delta < 0` can never be true. | Subtract signed values; leave duty unchanged at zero error. |

The integer filter is `y += (x-y)/16`, with division truncating toward zero.
It can stop up to 15 ADC counts from its input. Independent channels fed 1000
and 2000 settle at 985 and 1985 in this reference. That is permitted
quantisation, not a ninth planted defect. The harness checks tolerance and
monotonic movement rather than demanding exact equality.

At 125 MHz, divider 250 and 20 Hz, the correct wrap is 24999. The broken 25000
gives approximately 19.9992 Hz. The host arithmetic test is the reliable way to
detect such a small error; a scope needs sufficient accuracy to distinguish it.

The reference runs acquisition/logging in main to avoid printf in a timer ISR.
Its sample interval includes processing time. Sampling a 20 Hz square wave near
40 samples/s still aliases it; this demonstration is not a precision DC feedback
controller. Test ADC channel identity with two known DC voltages, and measure
PWM separately. Change the sampling rate when exploring aliasing.
