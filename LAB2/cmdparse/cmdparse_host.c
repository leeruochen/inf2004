/*
 * LAB 2 CHALLENGE - laptop harness for the command parser.
 * INF2004 LAB 2
 *
 *   gcc -Wall -Wextra -o cmdparse_host cmdparse_host.c cmdparse.c
 *   ./cmdparse_host
 *
 * Every vector below is hand-derived from the specification at the top of
 * cmdparse.h. They are correct. Do not edit them to make your code pass -
 * that is not a fix, it is a cover-up, and it is obvious in marking.
 *
 * Get this to print ALL TESTS PASS on your laptop before you go anywhere
 * near the Pico. A test loop you can run in one second is worth more than
 * any amount of staring at a serial terminal.
 */

#include <stdio.h>
#include <string.h>
#include "cmdparse.h"

static int failures = 0;

static void ok(const char *what, bool cond)
{
    if (!cond) failures++;
    printf("  %-46s %s\n", what, cond ? "ok" : "FAIL");
}

/* ------------------------------------------------------------------ */
static void test_split(void)
{
    char       *argv[CMD_MAX_ARGS];
    char        line[CMD_MAX_LINE + 1];
    int         n;

    printf("cmd_split\n");

    strcpy(line, "SET LED 3 ON");
    n = cmd_split(line, argv, CMD_MAX_ARGS);
    ok("\"SET LED 3 ON\" -> 4 words", n == 4);
    if (n == 4)
        ok("  words are SET/LED/3/ON",
           !strcmp(argv[0], "SET") && !strcmp(argv[1], "LED") &&
           !strcmp(argv[2], "3")   && !strcmp(argv[3], "ON"));

    strcpy(line, "  SET   LED  3   ON  ");
    n = cmd_split(line, argv, CMD_MAX_ARGS);
    ok("ragged spacing collapses to 4 words", n == 4);
    if (n == 4)
        ok("  words still SET/LED/3/ON",
           !strcmp(argv[0], "SET") && !strcmp(argv[1], "LED") &&
           !strcmp(argv[2], "3")   && !strcmp(argv[3], "ON"));

    strcpy(line, "PING");
    ok("\"PING\" -> 1 word", cmd_split(line, argv, CMD_MAX_ARGS) == 1);

    strcpy(line, "");
    ok("\"\" -> 0 words", cmd_split(line, argv, CMD_MAX_ARGS) == 0);

    strcpy(line, "     ");
    ok("all spaces -> 0 words", cmd_split(line, argv, CMD_MAX_ARGS) == 0);

    strcpy(line, "A B C D E");
    ok("5 words with max 4 -> -1", cmd_split(line, argv, CMD_MAX_ARGS) == -1);

    /* in-place: the caller's buffer must have been cut up, not copied */
    strcpy(line, "GET TEMP");
    n = cmd_split(line, argv, CMD_MAX_ARGS);
    ok("tokenised IN PLACE (argv points into line)",
       n == 2 && argv[0] == line && argv[1] > line && argv[1] < line + 8);
}

/* ------------------------------------------------------------------ */
static void test_parse_u8(void)
{
    uint8_t v;

    printf("cmd_parse_u8\n");
    ok("\"0\" -> 0",       cmd_parse_u8("0", &v)   && v == 0);
    ok("\"7\" -> 7",       cmd_parse_u8("7", &v)   && v == 7);
    ok("\"007\" -> 7",     cmd_parse_u8("007", &v) && v == 7);
    ok("\"255\" -> 255",   cmd_parse_u8("255", &v) && v == 255);
    ok("\"256\" rejected", !cmd_parse_u8("256", &v));
    ok("\"999\" rejected", !cmd_parse_u8("999", &v));
    ok("\"\" rejected",    !cmd_parse_u8("", &v));
    ok("\"12x\" rejected", !cmd_parse_u8("12x", &v));
    ok("\"x12\" rejected", !cmd_parse_u8("x12", &v));
    ok("\"-1\" rejected",  !cmd_parse_u8("-1", &v));
    ok("\" 7\" rejected",  !cmd_parse_u8(" 7", &v));
}

/* ------------------------------------------------------------------ */
static void test_word_eq(void)
{
    printf("cmd_word_eq\n");
    ok("\"ON\" == \"ON\"",      cmd_word_eq("ON", "ON"));
    ok("\"ON\" == \"on\"",      cmd_word_eq("ON", "on"));
    ok("\"Off\" == \"oFF\"",    cmd_word_eq("Off", "oFF"));
    ok("\"ON\" != \"OFF\"",    !cmd_word_eq("ON", "OFF"));
    ok("\"ON\" != \"ONE\"",    !cmd_word_eq("ON", "ONE"));
    ok("\"ONE\" != \"ON\"",    !cmd_word_eq("ONE", "ON"));
    ok("\"\" == \"\"",          cmd_word_eq("", ""));
}

/* ------------------------------------------------------------------ */
static cmd_status_t parse_str(const char *text, command_t *c)
{
    char line[CMD_MAX_LINE + 1];
    memset(line, 0, sizeof line);
    strncpy(line, text, CMD_MAX_LINE);
    return cmd_parse(line, c);
}

static void test_parse(void)
{
    command_t c;

    printf("cmd_parse - valid\n");
    memset(&c, 0, sizeof c);
    ok("\"PING\"",             parse_str("PING", &c) == CMD_OK && c.verb == CMD_PING);
    ok("\"ping\" (any case)",  parse_str("ping", &c) == CMD_OK && c.verb == CMD_PING);
    ok("\"GET TEMP\"",         parse_str("GET TEMP", &c) == CMD_OK && c.verb == CMD_GET_TEMP);

    memset(&c, 0, sizeof c);
    ok("\"SET LED 3 ON\"",     parse_str("SET LED 3 ON", &c) == CMD_OK &&
                               c.verb == CMD_SET_LED && c.led_index == 3 && c.led_on);
    memset(&c, 0, sizeof c);
    ok("\"SET LED 0 OFF\"",    parse_str("SET LED 0 OFF", &c) == CMD_OK &&
                               c.verb == CMD_SET_LED && c.led_index == 0 && !c.led_on);
    memset(&c, 0, sizeof c);
    ok("\"set led 2 off\"",    parse_str("set led 2 off", &c) == CMD_OK &&
                               c.verb == CMD_SET_LED && c.led_index == 2 && !c.led_on);
    memset(&c, 0, sizeof c);
    ok("\"  SET  LED  1  ON \"", parse_str("  SET  LED  1  ON ", &c) == CMD_OK &&
                               c.led_index == 1 && c.led_on);

    printf("cmd_parse - rejected\n");
    ok("\"\" -> EMPTY",              parse_str("", &c)              == CMD_ERR_EMPTY);
    ok("\"    \" -> EMPTY",          parse_str("    ", &c)          == CMD_ERR_EMPTY);
    ok("\"SETLED 3 ON\" -> VERB",    parse_str("SETLED 3 ON", &c)   == CMD_ERR_UNKNOWN_VERB);
    ok("\"BLINK\" -> VERB",          parse_str("BLINK", &c)         == CMD_ERR_UNKNOWN_VERB);
    ok("\"PING EXTRA\" -> COUNT",    parse_str("PING EXTRA", &c)    == CMD_ERR_ARG_COUNT);
    ok("\"GET\" -> COUNT",           parse_str("GET", &c)           == CMD_ERR_ARG_COUNT);
    ok("\"SET LED 3\" -> COUNT",     parse_str("SET LED 3", &c)     == CMD_ERR_ARG_COUNT);
    ok("\"SET LED 3 ON X\" -> COUNT",parse_str("SET LED 3 ON X", &c)== CMD_ERR_ARG_COUNT);
    ok("\"GET VOLTS\" -> VALUE",     parse_str("GET VOLTS", &c)     == CMD_ERR_ARG_VALUE);
    ok("\"SET PIN 3 ON\" -> VALUE",  parse_str("SET PIN 3 ON", &c)  == CMD_ERR_ARG_VALUE);
    ok("\"SET LED x ON\" -> VALUE",  parse_str("SET LED x ON", &c)  == CMD_ERR_ARG_VALUE);
    ok("\"SET LED 3 MAYBE\" -> VALUE", parse_str("SET LED 3 MAYBE", &c) == CMD_ERR_ARG_VALUE);
    ok("\"SET LED 4 ON\" -> RANGE",  parse_str("SET LED 4 ON", &c)  == CMD_ERR_ARG_RANGE);
    ok("\"SET LED 200 ON\" -> RANGE",parse_str("SET LED 200 ON", &c)== CMD_ERR_ARG_RANGE);
}

/* ------------------------------------------------------------------ */
static int feed_str(cmd_reader_t *r, const char *s)
{
    int last = 0;
    for (const char *p = s; *p; p++)
        last = cmd_reader_feed(r, *p);
    return last;
}

static void test_reader(void)
{
    cmd_reader_t r;

    printf("cmd_reader_feed\n");

    cmd_reader_reset(&r);
    ok("\"PING\\r\\n\" -> line ready", feed_str(&r, "PING\r\n") == 1);
    ok("  buffer holds \"PING\" (no CR)", !strcmp(r.buf, "PING"));

    cmd_reader_reset(&r);
    ok("\"PING\\n\" (bare LF) -> line ready", feed_str(&r, "PING\n") == 1);
    ok("  buffer holds \"PING\"", !strcmp(r.buf, "PING"));

    cmd_reader_reset(&r);
    ok("no terminator yet -> 0", feed_str(&r, "SET LED 3 ON") == 0);

    cmd_reader_reset(&r);
    ok("empty line \"\\n\" -> line ready", feed_str(&r, "\n") == 1);
    ok("  buffer is empty string", r.buf[0] == '\0');

    /* exactly CMD_MAX_LINE characters is legal; one more is not */
    {
        char exact[CMD_MAX_LINE + 3];
        memset(exact, 'A', CMD_MAX_LINE);
        exact[CMD_MAX_LINE]     = '\n';
        exact[CMD_MAX_LINE + 1] = '\0';
        cmd_reader_reset(&r);
        ok("exactly CMD_MAX_LINE chars -> accepted", feed_str(&r, exact) == 1);
        ok("  all characters kept", strlen(r.buf) == CMD_MAX_LINE);
    }
    {
        char over[CMD_MAX_LINE + 20];
        memset(over, 'A', CMD_MAX_LINE + 10);
        over[CMD_MAX_LINE + 10] = '\n';
        over[CMD_MAX_LINE + 11] = '\0';
        cmd_reader_reset(&r);
        ok("one char too many -> -1", feed_str(&r, over) == -1);

        /* and the reader must still work afterwards - this is the real test */
        ok("recovers: next line parses", feed_str(&r, "PING\r\n") == 1);
        ok("  buffer holds \"PING\"", !strcmp(r.buf, "PING"));
    }

    /* two commands back to back through one reader */
    cmd_reader_reset(&r);
    ok("first of two lines", feed_str(&r, "GET TEMP\r\n") == 1);
    ok("  holds \"GET TEMP\"", !strcmp(r.buf, "GET TEMP"));
    ok("second of two lines", feed_str(&r, "SET LED 1 OFF\r\n") == 1);
    ok("  holds \"SET LED 1 OFF\"", !strcmp(r.buf, "SET LED 1 OFF"));
}

/* ------------------------------------------------------------------ */
int main(void)
{
    printf("LAB 2 CHALLENGE - ASCII command parser\n\n");

    test_split();
    test_parse_u8();
    test_word_eq();
    test_parse();
    test_reader();

    printf("\n%s  (%d failure%s)\n",
           failures ? "NOT DONE YET" : "ALL TESTS PASS",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
