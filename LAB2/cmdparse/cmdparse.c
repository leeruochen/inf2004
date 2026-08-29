/*
 * LAB 2 CHALLENGE - an ASCII command parser.
 * INF2004 LAB 2
 *
 * The specification is at the top of cmdparse.h. It is correct.
 * Your job is to make this file match it.
 *
 *   gcc -Wall -Wextra -o cmdparse_host cmdparse_host.c cmdparse.c
 *   ./cmdparse_host
 *
 * Five functions. cmd_word_eq() is done for you as a worked example of the
 * style expected: no library string functions doing the thinking for you,
 * no allocation, no assumptions about the input.
 */

#include <string.h>
#include "cmdparse.h"

/* ------------------------------------------------------------------
 * WORKED EXAMPLE - compare two words ignoring ASCII case.
 *
 * Note what this does NOT do:
 *
 *   - It does not call toupper() from <ctype.h>. That function takes an
 *     int which must be representable as an unsigned char or EOF. A plain
 *     char is signed on ARM, so any byte above 0x7F arriving off the wire
 *     becomes negative and passing it to toupper() is undefined behaviour.
 *     One cast fixes it; forgetting the cast is a real and common bug.
 *
 *   - It does not copy either string anywhere. It walks both in place.
 *
 *   - It does not stop at the first difference in LENGTH: "ON" and "ONE"
 *     must compare unequal, so the final check is that BOTH strings ended
 *     at the same point.
 * ------------------------------------------------------------------ */
bool cmd_word_eq(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == *b;
}

/* ------------------------------------------------------------------
 * 1. Put a reader back into its starting state.
 * ------------------------------------------------------------------ */
void cmd_reader_reset(cmd_reader_t *r)
{
    (void)r;
    /* TODO */
}

/* ------------------------------------------------------------------
 * 2. Feed one received character.
 *
 *    Returns 1 (line ready), 0 (still accumulating), or -1 (that line
 *    was too long and has been discarded).
 *
 *    Three things make this harder than it looks, and all three are in
 *    the test harness:
 *      - a '\r' immediately before the '\n' is part of the terminator
 *      - EXACTLY CMD_MAX_LINE characters must be accepted, not rejected
 *      - after an overlong line, the NEXT line must still parse. You
 *        cannot simply stop writing once the buffer is full; you have to
 *        keep consuming until the terminator arrives, then report the
 *        error once and start clean.
 *
 *    Write down what the reader's state means before you write the code.
 *    This function is a small state machine, and Lab 3 is entirely about
 *    those.
 * ------------------------------------------------------------------ */
int cmd_reader_feed(cmd_reader_t *r, char c)
{
    (void)r; (void)c;
    /* TODO */
    return 0;
}

/* ------------------------------------------------------------------
 * 3. Split line into words, IN PLACE.
 *
 *    Return the number of words, or -1 if there are more than max_args.
 *    Write NULs over the separators; store pointers into the caller's
 *    buffer in argv[]. Do not copy the words anywhere.
 *
 *    The harness checks that argv[0] == line, so a version that copies
 *    will fail even if every word comes out right.
 * ------------------------------------------------------------------ */
int cmd_split(char *line, char *argv[], int max_args)
{
    (void)line; (void)argv; (void)max_args;
    /* TODO */
    return 0;
}

/* ------------------------------------------------------------------
 * 4. Parse an unsigned decimal number, strictly.
 *
 *    true only if the WHOLE string is digits and the value fits in a
 *    uint8_t. Write *out only on success.
 *
 *    Think about where you test for overflow. Accumulating the whole
 *    number and then checking whether it is greater than 255 works only
 *    if the accumulator is wide enough to hold what you accumulated -
 *    "99999999999" must be rejected, not wrapped.
 * ------------------------------------------------------------------ */
bool cmd_parse_u8(const char *s, uint8_t *out)
{
    (void)s; (void)out;
    /* TODO */
    return false;
}

/* ------------------------------------------------------------------
 * 5. Parse a whole line into a command.
 *
 *    Split it, identify the verb, check the argument COUNT before you
 *    look at any argument, then validate each argument in turn.
 *    Return the most specific CMD_ERR_* code that applies - the harness
 *    distinguishes ARG_COUNT from ARG_VALUE from ARG_RANGE, because a
 *    device that answers "ERR" and nothing else is a device nobody can
 *    debug from the far end of a cable.
 * ------------------------------------------------------------------ */
cmd_status_t cmd_parse(char *line, command_t *out)
{
    (void)line; (void)out;
    /* TODO */
    return CMD_ERR_UNKNOWN_VERB;
}

/* ------------------------------------------------------------------
 * Implemented for you.
 * ------------------------------------------------------------------ */
const char *cmd_status_str(cmd_status_t s)
{
    switch (s) {
        case CMD_OK:                return "OK";
        case CMD_ERR_EMPTY:         return "ERR EMPTY";
        case CMD_ERR_TOO_LONG:      return "ERR TOO_LONG";
        case CMD_ERR_UNKNOWN_VERB:  return "ERR UNKNOWN_VERB";
        case CMD_ERR_ARG_COUNT:     return "ERR ARG_COUNT";
        case CMD_ERR_ARG_VALUE:     return "ERR ARG_VALUE";
        case CMD_ERR_ARG_RANGE:     return "ERR ARG_RANGE";
    }
    return "ERR ?";
}
