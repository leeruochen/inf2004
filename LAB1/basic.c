#include <stdio.h>
#include "pico/stdlib.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main()
{
    stdio_init_all();

    printf("Hello, Embedded World!\n");

    while (true)
    {
        sleep_ms(3000);

        // Arithmetic operators
        printf("Arithmetic operators (a = 10, b = 20)\n");
        int a = 10, b = 20;
        printf("a + b = %d\n", a + b);  // 30
        printf("a - b = %d\n", a - b);  // -10
        printf("a * b = %d\n", a * b);  // 200
        printf("b / a = %d\n", b / a);  // 2
        printf("b %% a = %d\n", b % a); // 0

        // Relational operators
        printf("Relational operators\n");
        printf("a > b is %d\n", a > b);   // 0
        printf("a < b is %d\n", a < b);   // 1
        printf("a >= b is %d\n", a >= b); // 0
        printf("a <= b is %d\n", a <= b); // 1
        printf("a == b is %d\n", a == b); // 0
        printf("a != b is %d\n", a != b); // 1

        // Logical operators
        printf("Logical operators (x = 1, y = 0)\n");
        int x = 1, y = 0;
        printf("x && y is %d\n", x && y); // 0
        printf("x || y is %d\n", x || y); // 1
        printf("!x is %d\n", !x);         // 0

        // Increment and decrement operators
        printf("Increment and decrement operators (c = 5)\n");
        int c = 5;
        printf("c++ is %d\n", c++); // 5, this prints 5 as ++ behind c is post increment, so it prints the value of c before incrementing
        printf("++c is %d\n", ++c); // 7, this prints 7 as ++ before c is pre increment, so it increments the value of c before printing
        printf("++c is %d\n", ++c); // 8
        printf("c-- is %d\n", c--); // 8, this prints 8 as -- behind c is post decrement, so it prints the value of c before decrementing
        printf("--c is %d\n", --c); // 6, this prints 6 as -- before c is pre decrement, so it decrements the value of c before printing

        // Assignment operators
        printf("Assignment operators (d = 10)\n");
        int d = 10;
        d += 5;
        printf("d after += 5: %d\n", d); // 15

        // Ternary conditional operator
        printf("Ternary conditional operators (someNumber = 15)\n");
        int someNumber = 15;
        const char *result = (someNumber > 10) ? "Greater than 10" : "Not greater than 10";
        printf("Ternary result: %s\n", result);

        // Bitwise operators
        printf("Bitwise operators (p = 5, q = 3)\n");
        int p = 5, q = 3;
        printf("p & q = %d\n", p & q);   // Bitwise AND
        printf("p | q = %d\n", p | q);   // Bitwise OR
        printf("p ^ q = %d\n", p ^ q);   // Bitwise XOR
        printf("~p = %d\n", ~p);         // Bitwise NOT, prints -6
        printf("q << 1 = %d\n", q << 1); // Left shift
        printf("q >> 1 = %d\n", q >> 1); // Right shift

        // Macros
        printf("Macros (num1 = 10, num2 = 15)\n");
        int num1 = 10, num2 = 15;
        int max = MAX(num1, num2);
        printf("The maximum of %d and %d is %d\n", num1, num2, max);
    }
    return 0;
}
