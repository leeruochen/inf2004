# Debug Logbook — Bug Hunt #1

**Name(s):** 
**Date:** 

One row per hypothesis. **Include the hypotheses that turned out to be wrong** —
they are the evidence that you were reasoning rather than guessing.

| # | Symptom observed | Hypothesis | Experiment (ONE change) | Predicted | Result | Conclusion |
|---|---|---|---|---|---|---|
| 1 | unable to compile due to unknown type name 'uint8_t'| missing library | add #include <stdint.h>| now the missing types will be known| errors related to missing library are gone| missing library was the issue|
| 2 | unable to compile due to missing ;| syntax error| add ; required| error will be gone| error is gone| be mindful for missing ; especially in c|
| 3 | missing | | | | | |
| 4 | | | | | | |
| 5 | | | | | | |
| 6 | | | | | | |
| 7 | | | | | | |
| 8 | | | | | | |

---

## Defects found

| # | File & line | Class (syntax / logical / heisenbug / UB) | The defect | The fix | Why the fix is correct |
|---|---|---|---|---|---|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |
| 4 | | | | | |
| 5 | | | | | |
| 6 | | | | | |

---

## Reflection

**Which defect took longest, and what finally cracked it?**



**Which instrument was most useful, and which one misled you?**



**What would have caught this defect automatically — a compiler flag, an
assertion, a test, a code review rule?**


