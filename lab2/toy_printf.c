/* toy-io.c
 * Limited versions of printf and scanf
 *
 * Programmer: Mayer Goldberg, 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* the states in the printf state-machine */
enum printf_state {
  st_printf_init,
  st_printf_percent,
  st_printf_octal2,
  st_printf_octal3,
  st_printf_array,
  st_printf_right_padding,
  st_printf_left_padding,
  st_printf_zero_padding
};

#define MAX_NUMBER_LENGTH 64
#define is_octal_char(ch) ('0' <= (ch) && (ch) <= '7')

int toy_printf(char *fs, ...);

const char *digit = "0123456789abcdef";
const char *DIGIT = "0123456789ABCDEF";

int print_int_helper(long n, int radix, const char *digit) {
  int result;

  if (n < radix) {
    putchar(digit[n]);
    return 1;
  }
  else {
    result = print_int_helper(n / radix, radix, digit);
    putchar(digit[n % radix]);
    return 1 + result;
  }
}

int print_int(long n, int radix, const char * digit) {
  if (radix < 2 || radix > 16) {
    toy_printf("Radix must be in [2..16]: Not %d\n", radix);
    exit(-1);
  }

  if (n > 0) {
    return print_int_helper(n, radix, digit);
  }
  if (n == 0) {
    putchar('0');
    return 1;
  }
  else {
    putchar('-');
    return 1 + print_int_helper(-n, radix, digit);
  }
}

int compute_int_length_helper(int n) {
  int result = 1;

  while (n >= 10) {
    result++;
    n /= 10;
  }
  
  return result;
}

int compute_int_length(int n) {
  if (n > 0) {
    return compute_int_length_helper(n);
  }
  if (n == 0) {
    return 1;
  }
  else {
    return 1 + compute_int_length_helper(-n);
  }
}

/* SUPPORTED:
 *   %b, %d, %u, %o, %x, %X -- 
 *     integers in binary, decimal, octal, hex, and HEX
 *   %s -- strings
 */

int toy_printf(char *fs, ...) {
  int chars_printed = 0;
  int int_value = 0;
  unsigned int unsigned_int_value = 0;
  char *string_value;
  char char_value;
  int *int_array;
  unsigned int *unsigned_int_array;
  char **string_array;
  char *char_array;
  int array_size;
  int width = 0;
  int int_length;
  int string_length;
  va_list args;
  enum printf_state state;

  va_start(args, fs);

  state = st_printf_init; 

  for (; *fs != '\0'; ++fs) {
    switch (state) {
    case st_printf_init:
      switch (*fs) {
      case '%':
  state = st_printf_percent;
  break;

      default:
  putchar(*fs);
  ++chars_printed;
      }
      break;

    case st_printf_percent:
      switch (*fs) {
      case '%':
  putchar('%');
  ++chars_printed;
  state = st_printf_init;
  break;

      case 'd':
  int_value = va_arg(args, int);
  chars_printed += print_int(int_value, 10, digit);
  state = st_printf_init;
  break;

      case 'u':
  unsigned_int_value = va_arg(args, unsigned int);
  chars_printed += print_int(unsigned_int_value, 10, digit);
  state = st_printf_init;
  break;

      case 'b':
  unsigned_int_value = va_arg(args, unsigned int);
  chars_printed += print_int(unsigned_int_value, 2, digit);
  state = st_printf_init;
  break;

      case 'o':
  unsigned_int_value = va_arg(args, unsigned int);
  chars_printed += print_int(unsigned_int_value, 8, digit);
  state = st_printf_init;
  break;
  
      case 'x':
  unsigned_int_value = va_arg(args, unsigned int);
  chars_printed += print_int(unsigned_int_value, 16, digit);
  state = st_printf_init;
  break;

      case 'X':
  unsigned_int_value = va_arg(args, unsigned int);
  chars_printed += print_int(unsigned_int_value, 16, DIGIT);
  state = st_printf_init;
  break;

      case 's':
  string_value = va_arg(args, char *);
  while (*string_value) {
    chars_printed++;
    putchar(*string_value);
    string_value++;
  }
  state = st_printf_init;
  break;

      case 'c':
  char_value = (char)va_arg(args, int);
  putchar(char_value);
  ++chars_printed;
  state = st_printf_init;
  break;

      case 'A':
  state = st_printf_array;
  break;

      case '-':
  state = st_printf_left_padding;
  break;

      case '0':
  state = st_printf_zero_padding;
  break;

      case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9':
  width = *fs - '0';
  state = st_printf_right_padding;
  break;

      default:
  toy_printf("Unhandled format %%%c...\n", *fs);
  exit(-1);
      }
      break;

    case st_printf_array:
      switch (*fs) {
      case 'd':
  chars_printed += toy_printf("{");
  int_array = va_arg(args, int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(int_array[i], 10, digit);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'u':
  chars_printed += toy_printf("{");
  unsigned_int_array = va_arg(args, unsigned int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(unsigned_int_array[i], 10, digit);
    if (i < array_size - 1) {
    chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'b':
  chars_printed += toy_printf("{");
  unsigned_int_array = va_arg(args, unsigned int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(unsigned_int_array[i], 2, digit);
    if (i < array_size - 1) {
    chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'o':
  chars_printed += toy_printf("{");
  unsigned_int_array = va_arg(args, unsigned int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(unsigned_int_array[i], 8, digit);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'x':
  chars_printed += toy_printf("{");
  unsigned_int_array = va_arg(args, unsigned int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(unsigned_int_array[i], 16, digit);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'X':
  chars_printed += toy_printf("{");
  unsigned_int_array = va_arg(args, unsigned int *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += print_int(unsigned_int_array[i], 16, DIGIT);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 's':
  chars_printed += toy_printf("{");
  string_array = va_arg(args, char **);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += toy_printf("\"%s\"", string_array[i]);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      case 'c':
  chars_printed += toy_printf("{");
  char_array = va_arg(args, char *);
  array_size = va_arg(args, int);
  for (int i = 0; i < array_size; i++) {
    chars_printed += toy_printf("'%c'", char_array[i]);
    if (i < array_size - 1) {
      chars_printed += toy_printf(", ");
    }
  }
  chars_printed += toy_printf("}");
  state = st_printf_init;
  break;

      default:
  toy_printf("Unhandled format %%A%c...\n", *fs);
  exit(-1);
      }
      break;

    case st_printf_right_padding:
      switch (*fs) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
  width = (10 * width) + (*fs - '0');
  break;

      case 'd':
  int_value = va_arg(args, int);
  int_length = print_int(int_value, 10, digit);
  chars_printed += int_length;
  width -= int_length;
  while (width > 0) {
    chars_printed += toy_printf(" ");
    width--;
  }
  chars_printed += toy_printf("#");
  state = st_printf_init;
  break;

      case 's':
  string_value = va_arg(args, char *);
  while (*string_value) {
    chars_printed++;
    putchar(*string_value);
    string_value++;
      width--;
  }
  while (width > 0) {
    chars_printed += toy_printf(" ");
    width--;
  }
  chars_printed += toy_printf("#");
  state = st_printf_init;
  break;

      default:
  toy_printf("Unhandled format %%<width>%c...\n", *fs);
  exit(-1);
      }
      break;

    case st_printf_left_padding:
      switch (*fs) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
  width = (10 * width) + (*fs - '0');
  break;

      case 'd':
  int_value = va_arg(args, int);
  int_length = compute_int_length(int_value);
  width -= int_length;
  while (width > 0) {
    chars_printed += toy_printf(" ");
    width--;
  }
  chars_printed += print_int(int_value, 10, digit);
  state = st_printf_init;
  break;

      case 's':
  string_value = va_arg(args, char *);
  string_length = 0;
  while (*string_value) {
    string_value++;
    string_length++;
  }
  string_value -= string_length;
  width -= string_length;
  while (width > 0) {
    chars_printed += toy_printf(" ");
    width--;
  }
  chars_printed += toy_printf("%s", string_value);
  state = st_printf_init;
  break;

      default:
  toy_printf("Unhandled format %%-<width>%c...\n", *fs);
  exit(-1);
      }
      break;

    case st_printf_zero_padding:
      switch (*fs) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
  width = (10 * width) + (*fs - '0');
  break;

      case 'd':
  int_value = va_arg(args, int);
  int_length = compute_int_length(int_value);
  width -= int_length;
  if (int_value >= 0) {
    while (width > 0) {
      chars_printed += toy_printf("0");
      width--;
    }
    chars_printed += print_int(int_value, 10, digit);
  }
  else {
    chars_printed += toy_printf("-");
    while (width > 0) {
      chars_printed += toy_printf("0");
      width--;
    }
    chars_printed += print_int(-int_value, 10, digit);
  }
  state = st_printf_init;
  break;

      default:
  toy_printf("Unhandled format %%0<width>%c...\n", *fs);
  exit(-1);
      }
      break;

    default:
      toy_printf("toy_printf: Unknown state -- %d\n", (int)state);
      exit(-1);
    }
  }

  va_end(args);

  return chars_printed;
}


