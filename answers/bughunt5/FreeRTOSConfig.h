/* Reuse the exercise configuration, enabling diagnostics for this reference. */
#include "../../LAB5/bughunt/FreeRTOSConfig.h"
#undef configASSERT
#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for (;;) {} } } while (0)
#undef configCHECK_FOR_STACK_OVERFLOW
#define configCHECK_FOR_STACK_OVERFLOW 2
#undef configUSE_MALLOC_FAILED_HOOK
#define configUSE_MALLOC_FAILED_HOOK 1
