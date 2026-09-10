# Answer 5 — FreeRTOS sensor pipeline

[Exercise](../../LAB5/bughunt/README.md) · [Corrected source](bughunt5.c) · [Diagnostic config](FreeRTOSConfig.h) · [Build instructions](../README.md)

| Defect / investigation | Why it fails | Reference approach |
|---|---|---|
| `sizeof(&sample)` | Sends four bytes from a two-byte sample on RP2040. The receive buffers cannot hold that message. | Send `sizeof sample`. |
| Unprotected shared ring | Two tasks can interleave multi-step updates. | Give the moving-average task sole ownership of its ring. A mutex is unnecessary once sharing is removed. |
| Static mean state shared by callers | Both tasks update one history. | Give the running-mean task its own accumulator and count. |
| Print-task stack margin | Formatting needs stack, but overflow depends on the library and build. | Enable overflow checking and report high-water marks. The reference allocates 1024 words; verify the margin on hardware. |
| Wrong sample interval | `vTaskDelay(1)` is one tick; the 100 ms constant is unused. | Use `vTaskDelayUntil` and `pdMS_TO_TICKS(100)` to avoid cumulative processing-time drift. |
| Two readers of one message buffer | Unsynchronised reads violate its contract; even serialised reads distribute messages rather than broadcast them. | Send each sample to two independent single-reader buffers. |
| Ignored send failure | A full queue silently discards output. | Block task-level print sends; check sensor sends and stop visibly if acquisition cannot keep up. |
| Divide by capacity during warm-up | The first nine averages include slots with no sample. | Divide by the number of samples actually stored. |
| Disabled assertions | Kernel precondition failures are invisible. This is not itself C undefined behaviour. | Enable assertions, allocation and stack-overflow hooks. |

Initially the broken consumers may spin without producing temperature output:
FreeRTOS leaves the oversized first message queued and returns zero. Fix the
message size before attempting to observe later pipeline behaviour. See the
[FreeRTOS message-buffer contract](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/message_buffer.h).

After fan-out, do not let both consumers push into the same ring: that would
duplicate every reading and change the ten-sample window. The running-mean
consumer does not need a ring. The reference uses a 64-bit sum and count with
overflow checks, converting to floating point only when reporting the mean.

Only print_task calls printf. Health reporting runs there too, so no timer
callback blocks on the print queue. A stalled output eventually triggers the
sensor-send assertion rather than silently losing samples. No finite buffer
can guarantee lossless periodic acquisition through an unlimited downstream stall.

For a deterministic hardware check, temporarily publish samples 10,20,...,100
(tenths of a degree). The first moving average is 1.0 C; after ten samples both
means are 5.5 C. Publish 110: moving average is 6.5 C, running mean is 6.0 C.
Check the 100 ms cadence and the high-water table over sustained operation.
Do not claim stack overflow unless the hook, debugger or measurements establish it.
