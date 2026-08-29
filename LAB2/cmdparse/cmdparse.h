/*
 * LAB 2 CHALLENGE - an ASCII command parser.
 * INF2004 LAB 2
 *
 * Bug Hunt #2 puts a sensor reading on the wire as BYTES. This puts commands
 * on the same wire as TEXT. Both are real wire formats; almost every device
 * you will ever talk to uses one or the other, and a great many use both.
 *
 * ==================================================================
 * SPECIFICATION - this is correct. Your implementation must match it.
 * ==================================================================
 *
 * THE LINE PROTOCOL
 *
 *   A command is a line of ASCII terminated by "\r\n" or by a bare "\n".
 *   A line holds at most CMD_MAX_LINE characters before the terminator.
 *   A longer line is an error, and the parser must survive it: no overflow,
 *   no corruption, and the NEXT line must parse normally.
 *
 * THE COMMANDS
 *
 *   PING                     -> reply "OK"
 *   GET TEMP                 -> reply "OK <temperature>"
 *   SET LED <index> <state>  -> reply "OK"
 *
 *     <index>  decimal, 0 to 3 inclusive
 *     <state>  ON or OFF
 *
 *   Verbs and arguments are CASE-INSENSITIVE. "set led 3 on" is valid.
 *   Words are separated by one or more spaces. Leading and trailing spaces
 *   are not an error and are not significant.
 *
 * WHAT IS NOT ALLOWED
 *
 *   - No malloc. Not one byte. This runs on a microcontroller with 264 KB of
 *     SRAM and no way to recover from a leak.
 *   - No strtok(). It keeps hidden state in a static variable, which makes it
 *     unusable the moment you have two links or an interrupt. Split the line
 *     yourself.
 *   - No fixed-size copies of the input. Tokenise IN PLACE: write NULs over
 *     the separators in the caller's buffer and hand back pointers into it.
 *     This is the whole reason the parser takes a writable char*.
 *
 * ==================================================================
 *
 * Build and test on your laptop - do this before you touch hardware:
 *   gcc -Wall -Wextra -o cmdparse_host cmdparse_host.c cmdparse.c
 *   ./cmdparse_host
 *
 * Build for the Pico: see CMakeLists.txt in this folder.
 */
#ifndef CMDPARSE_H
#define CMDPARSE_H

#include <stdint.h>
#include <stdbool.h>

#define CMD_MAX_LINE  64u   /* characters in a line, excluding terminator */
#define CMD_MAX_ARGS   4u   /* the longest command is SET LED <n> <state> */

#define CMD_LED_MIN    0u
#define CMD_LED_MAX    3u

typedef enum {
    CMD_PING,
    CMD_GET_TEMP,
    CMD_SET_LED,
} cmd_verb_t;

typedef enum {
    CMD_OK = 0,
    CMD_ERR_EMPTY,          /* the line held no words at all              */
    CMD_ERR_TOO_LONG,       /* the line exceeded CMD_MAX_LINE             */
    CMD_ERR_UNKNOWN_VERB,   /* first word is not PING, GET or SET         */
    CMD_ERR_ARG_COUNT,      /* right verb, wrong number of words          */
    CMD_ERR_ARG_VALUE,      /* an argument is not one of the valid words  */
    CMD_ERR_ARG_RANGE,      /* a numeric argument is outside its range    */
} cmd_status_t;

typedef struct {
    cmd_verb_t verb;
    uint8_t    led_index;   /* meaningful only when verb == CMD_SET_LED */
    bool       led_on;      /* meaningful only when verb == CMD_SET_LED */
} command_t;

/* ------------------------------------------------------------------
 * The line reader. Feed it one character at a time as it arrives from
 * the UART; it tells you when a whole line is ready.
 * ------------------------------------------------------------------ */
typedef struct {
    char    buf[CMD_MAX_LINE + 1];  /* +1 for the NUL you must write */
    uint8_t len;
    bool    dropping;               /* this line already overflowed  */
} cmd_reader_t;

/* Put a reader back into its starting state. Safe to call at any time. */
void cmd_reader_reset(cmd_reader_t *r);

/*
 * Feed one received character.
 *   returns  1  a complete line is in r->buf, NUL-terminated, ready to parse
 *   returns  0  still accumulating, nothing to do
 *   returns -1  that line was too long; it has been discarded and the reader
 *               is ready for the next one
 *
 * A '\r' immediately before a '\n' is part of the terminator and must not
 * appear in r->buf. A lone '\n' also terminates the line.
 */
int cmd_reader_feed(cmd_reader_t *r, char c);

/* ------------------------------------------------------------------
 * The pieces. Each is separately testable, which is why they are
 * separate - see cmdparse_host.c.
 * ------------------------------------------------------------------ */

/*
 * Split line into words, IN PLACE.
 *
 * Overwrites each run of separators with a NUL and stores a pointer to the
 * start of each word in argv[]. Returns the number of words found, or -1 if
 * there are more than max_args of them.
 *
 *   "  SET   LED  3   ON  "  ->  4, argv = {"SET", "LED", "3", "ON"}
 *   "PING"                   ->  1, argv = {"PING"}
 *   "   "                    ->  0
 */
int cmd_split(char *line, char *argv[], int max_args);

/*
 * Parse an unsigned decimal number. Stricter than atoi(), which cannot
 * report failure and is therefore useless for input that came off a wire.
 *
 * Returns true only if the whole string is digits and the value fits in a
 * uint8_t. Writes *out only on success.
 *
 *   "0" -> 0    "007" -> 7    "255" -> 255
 *   "256" fails    "" fails    "12x" fails    "-1" fails    " 7" fails
 */
bool cmd_parse_u8(const char *s, uint8_t *out);

/*
 * Compare two words ignoring ASCII case. Returns true when they match.
 *   cmd_word_eq("ON", "on") is true
 */
bool cmd_word_eq(const char *a, const char *b);

/*
 * Parse a whole line into a command. The line is modified in place.
 * Returns CMD_OK and fills *out, or one of the CMD_ERR_* codes.
 */
cmd_status_t cmd_parse(char *line, command_t *out);

/* Human-readable name for a status code. Implemented for you. */
const char *cmd_status_str(cmd_status_t s);

#endif
