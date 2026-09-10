# Bug hunt answers

These are ungraded practice exercises. Try the hunt first, then use these
explanations and complete reference programs to compare your reasoning. There
is more than one valid solution; understand why a change works before copying it.
The deliberately broken programs remain in the lab folders.

| Hunt | Explanation and corrected source |
|---|---|
| 1 — bits | [Answer](bughunt1/README.md) |
| 2 — frames and UART | [Answer](bughunt2/README.md) |
| 3 — interrupts | [Answer](bughunt3/README.md) |
| 4 — ADC, filter and PWM | [Answer](bughunt4/README.md) |
| 5 — FreeRTOS pipeline | [Answer](bughunt5/README.md) |
| 6 — telemetry and debugging | [Answer](bughunt6/README.md) |

## Run the host answers

From the repository root, with GCC and CMake installed:

```bash
cmake -S answers -B build-answers
cmake --build build-answers
ctest --test-dir build-answers --output-on-failure
```

Hunts 1, 2, 4 and 6 use the student harnesses, not separate easier tests.
For memory and undefined-behaviour checks, use a separate build:

```bash
cmake -S answers -B build-answers-sanitize -DCMAKE_C_FLAGS="-g -fsanitize=address,undefined"
cmake --build build-answers-sanitize
ctest --test-dir build-answers-sanitize --output-on-failure
```

## Build a Pico answer

Select one hunt per build directory; this example builds #2 for both Picos:

```bash
cmake -S answers -B build-answer2-pico -DANSWER_PICO=2 \
  -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DCMAKE_BUILD_TYPE=Debug -DPICO_DEOPTIMIZED_DEBUG=1
cmake --build build-answer2-pico
```

Flash `build-answer2-pico/bughunt2_answer.uf2`. Use the lab's wiring. For #5,
also supply `-DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel`. The RP2040 port
must exist at `portable/ThirdParty/GCC/RP2040`; record your kernel revision.
Pico SDK 1.5.1 defaults Debug to `-Og`; `PICO_DEOPTIMIZED_DEBUG=1` selects `-O0`.

## Validation and limits

The host references are checked with GCC 13.3.0. Pico builds are checked with
ARM GCC 13.2.1 and SDK 1.5.1; the FreeRTOS checkout used for #5 is `b4005374c`.
LeakSanitizer cannot run under some ptrace-based sandboxes. In that environment,
use `ASAN_OPTIONS=detect_leaks=0` for CTest; this leaves address and undefined-
behaviour checks enabled. Leak checking was unavailable in the validation sandbox.

Compilation is not a hardware test. UART wiring/recovery, encoder timing and
FreeRTOS stack margins must still be checked on your boards using each answer's
suggested experiments. Undefined behaviour does not have a guaranteed symptom.
