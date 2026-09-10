# Answer 3 — interrupt state and timing

[Exercise](../../LAB3/bughunt/README.md) · [Corrected source](bughunt3.c) · [Build instructions](../README.md)

| Defect | Why it fails | Correction |
|---|---|---|
| Shared state not volatile | The optimiser can reuse the old pulse count forever. | Mark ISR-written/main-read state `volatile` on this single-core target. |
| Non-atomic 64-bit timestamp | Main can read two halves from different updates. | Snapshot all reported fields with interrupts disabled, then restore the previous interrupt state. |
| Deadline addition wraps | `last + delay` may overflow before the comparison. | Compare unsigned elapsed time: `now - last > delay`. |
| Excessive debounce time | A 50 ms exclusion interval rejects real encoder edges. | Choose a delay below the shortest legitimate high and low intervals. The reference's 1 ms is for slow manual rotation. |
| Switch fall-through | One edge starts and immediately finishes a slot. | Separate the start and finish transitions. |
| Printing in the ISR | USB output adds latency and may block; edges may be lost. | Save measurements in the ISR and print from main. |
| Falling edges only | A pulse-width measurement requires its start and end. | Enable both edges; use `events` to distinguish them. |
| Wrong printf argument type | `%u` cannot format a 64-bit timestamp. | `%llu` with an `unsigned long long` cast. |

The reference measures complete low pulses, discards an initial unmatched rise,
and restarts if both edge flags have accumulated before service. Reverse polarity
if you specifically need high-pulse widths. RPM uses the interval between two
slot starts, including the gap; width alone cannot establish rotational speed.

Build the broken exercise at `-O0` and `-O3`, then compare the wait-loop loads.
Build the answer at both levels: its pulse-count load must remain inside the
loop. On hardware, feed known high/low intervals and verify count, width and
period. Twenty periods per second with 20 slots/revolution is 60 RPM, regardless
of the high/low duty ratio.

For rollover, test the arithmetic with synthetic timestamps. For example,
`last=0xFFFFF000`, `now=0xFFFFF100`, `delay=50000`: only 256 us elapsed, but the
broken addition test accepts it. Setting `last` alone does not advance the clock.
Do not expect removing a printf to expose another fault on every run; absence
of an intermittent symptom is a valid observation, not a failed exercise.
