# Answer 1 — bit manipulation

[Exercise](../../LAB1/bughunt/README.md) · [Corrected source](bughunt1.c) · [Build instructions](../README.md)

| Defect | Why it fails | Correction |
|---|---|---|
| Missing `<stdint.h>` | The host compiler has no declaration for the fixed-width types. | Include the header explicitly. |
| Missing semicolon | Parsing fails at the following `while`. | End the `count` declaration with `;`. |
| Missing closing brace | `else` arrives before the `if` body ends. | Close the body before `else`. |
| Signed bit-count input | On these GCC targets, shifting negative `int` right extends its sign; `-1` never becomes zero. | Use `uint32_t`; the converted `-1` has 32 set bits. |
| Mask-test precedence | `==` binds before `&`, so the mask is ANDed with a Boolean. | `(mask & (1u << pin)) == 0`. Valid pin numbers here are 0..31. |
| Bit-reversal loop bound | Iteration 32 shifts by 32 and by -1, both undefined. | Iterate with `i < 32`. |

`swap_nibbles` was already correct. Do not change code just because it appears
in a bug hunt. Negative signed right shift is implementation-defined in the C
versions used here; sign extension describes these toolchains, not every C target.

The corrected program must reproduce [expected.txt](../../LAB1/bughunt/expected.txt)
exactly and report zero failures, including under the sanitizers.
