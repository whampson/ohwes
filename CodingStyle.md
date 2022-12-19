# Coding Style Guide
It's essentially the Linux kernel style guide, with some modifications.

## 1. Indentation and Line Etiquette
Please use spaces, tabs are not allowed. Use 4 characters per indent. Try not to
exceed 3 indentation levels.

I prefer switch statements to look as follows. Therefore, here you can violate
the 3 indentation level rule when using switch statements. Why? I think this is
easier to read. You shouldn't be doing a whole lot in each case anyway.

```c
switch (opt) {
    case 'a':
        val = 42;
        break;
    case 'b':
        val = Calculate();
        break;
    case 'c':
    case 'd':
        val *= 17;
        break;
    default:
        printf("unrecognized option\n");
        break;
}
```

Try to keep code under 80 characters per line. Do not under any circumstances
exceed 120 characters per line; fix your code instead. Break up long lines at
sensible locations to maintain readability. When breaking a line, place the
broken text below one indent level.

Strings are an exception to the above rule. Only break up strings at linefeed
characters to maintain readability and searchability of the string.

## 2. Spaces and Braces
### Spaces
Use a single space around binary and ternary operators.

```
=  +  -  <  >  *  /  %  |  &  ^  <=  >=  ==  !=  ?  :
```

Use a single space after these keywords.

```
if  switch  for  case  do  while
```

Do not use a space before or after unary operators.
```
&  *  +  -  ~  !  ++  --  sizeof offsetof static_assert
```

For function-like unary operators such as sizeof, offsetof, and static_assert,
it is preferred to surround the expression with parentheses.

```
s = sizeof(struct BiosParamBlock)
```

Do not us a space around structure member operators.

```
.  ->
```

Do not put a space after parentheses.

Please trim trailing whitespace.

Separate groups of code with a single empty line where appropriate.

### Braces
Put opening curly braces on the same line, except for functions, where the
opening brace goes on the following line; K&R Style.

Place closing curly braces on their own line, Stroustrup style. I think the
extra space of the closing curly brace improves readability of if..else chains
without creating too much empty space.

```c
int MyFunc(int x)
{
    if (x == 42) {
        x <<= 2;
    }
    else if (x == 100) {
        x *= 17;
    }
    else {
        x = Calculate(x);
    }

    return x;
}
```

However, do..while statements are an exception to this rule. The 'while' part
should go on the same line. I think it looks weird any other way.

```c
do {
    // stuff
} while (x > 14);
```

You may omit curly braces entirely for a single statement, but use caution when
doing so.

## 3. Naming
Use camelCase for local variables, PascalCase for constants, labels, structures,
and functions.

Use UPPER_CASE for macros that define constants.

(more coming soon)
