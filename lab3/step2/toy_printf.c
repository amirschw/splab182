/* toy-io.c
 * Limited versions of printf and scanf
 *
 * Programmer: Mayer Goldberg, 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* the states in the printf state-machine */
enum printf_state {
  st_printf_init,
  st_printf_percent,
  st_printf_array,
  st_printf_padding
};

typedef struct state_args {
  char* fs;
  int *int_array;
  unsigned int *unsigned_int_array;
  char **string_array;
  char *char_array;
  int width;
  char side;
} state_args;

typedef struct state_result {
  int printed_chars;
  enum printf_state next_state;
} state_result;

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

int print_int(long n, int radix, const char *digit) {
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

int print_string(char *str) {
  int res = 0;
  while (*str) {
    res++;
    putchar(*str);
    str++;
  }
  return res;
}

int print_char(char c) {
  putchar(c);
  return 1;
}

int print_int_array(int *arr, int arr_size, int radix, const char *digit) {
  int res = 0;

  res += toy_printf("{");
  for (int i = 0; i < arr_size; i++) {
    res += print_int(arr[i], radix, digit);
    if (i < arr_size - 1) {
      res += toy_printf(", ");
    }
  }
  res += toy_printf("}");
  
  return res;
}

int print_unsigned_int_array(unsigned int *arr, int arr_size, int radix, const char *digit) {
  int res = 0;

  res += toy_printf("{");
  for (int i = 0; i < arr_size; i++) {
    res += print_int(arr[i], radix, digit);
    if (i < arr_size - 1) {
      res += toy_printf(", ");
    }
  }
  res += toy_printf("}");
  
  return res;
}

int print_string_array(char **arr, int arr_size) {
  int res = 0;

  res += toy_printf("{");
  for (int i = 0; i < arr_size; i++) {
    res += print_string(arr[i]);
    if (i < arr_size - 1) {
      res += toy_printf(", ");
    }
  }
  res += toy_printf("}");
  
  return res;
}

int print_char_array(char *arr, int arr_size) {
  int res = 0;

  res += toy_printf("{");
  for (int i = 0; i < arr_size; i++) {
    res += print_char(arr[i]);
    if (i < arr_size - 1) {
      res += toy_printf(", ");
    }
  }
  res += toy_printf("}");

  return res;
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

void print_padding(int width, char pad) {
  while (width > 0) {
    putchar(pad);
    width--;
  }
}

int print_padded_int(int n, int width, char pad) {
  int res = width -= compute_int_length(n);

  switch (pad) {
    case 'r':
      res += print_int(n, 10, digit);
      print_padding(width, ' ');
      res += toy_printf("#");
      break;
    
    case 'l':
      print_padding(width, ' ');
      res += print_int(n, 10, digit);
      break;
    
    case 'z':
      if (n < 0) {
        res += toy_printf("-");
      }
      print_padding(width, '0');
      res += print_int(n >= 0 ? n : -n, 10, digit);
      break;
  }

  return res;
}

int print_padded_string(char *str, int width, char pad) {
  int res = width -= strlen(str);

  switch (pad) {
    case 'r':
      res += print_string(str);
      print_padding(width, ' ');
      res += toy_printf("#");
      break;

    case 'l':
      print_padding(width, ' ');
      res += print_string(str);
      break;
  }

  return res;
}

state_result init_handler(va_list args, state_args *state) {
  state_result res;
  switch (*(state->fs)) {
    case '%':
      res.printed_chars = 0;
      res.next_state = st_printf_percent;
      return res;

    default:
      putchar(*(state->fs));
      res.printed_chars = 1;
      res.next_state = st_printf_init;
      return res;
  }
}

state_result percent_handler(va_list args, state_args *state) {
  state_result res;
  res.printed_chars = 0;
  char c = *(state->fs);
  switch (c) {
    case '%':
      putchar('%');
      res.printed_chars++;
      res.next_state = st_printf_init;
      return res;
    
    case 'd': case 'u': case 'b': case 'o': case 'x': case 'X':
      res.printed_chars += print_int(c == 'd' ? va_arg(args, int) : va_arg(args, unsigned int),
                                    c == 'd' || c == 'u' ? 10 : c == 'b' ? 2 : c == 'o' ? 8 : 16, c == 'X' ? DIGIT : digit);
      res.next_state = st_printf_init;
      return res;

    case 's':
      res.printed_chars += print_string(va_arg(args, char *));
      res.next_state = st_printf_init;
      return res;

    case 'c':
      res.printed_chars += print_char((char)va_arg(args, int));
      res.next_state = st_printf_init;
      return res;

    case 'A':
      res.next_state = st_printf_array;
      return res;

    case '-':
      state->side = 'l';
      res.next_state = st_printf_padding;
      return res;

    case '0':
      state->side = 'z';
      res.next_state = st_printf_padding;
      return res;

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
      state->width = *(state->fs) - '0';
      state->side = 'r';
      res.next_state = st_printf_padding;
      return res;

    default:
      toy_printf("Unhandled format %%%c...\n", c);
      exit(-1);
  }
}

state_result array_handler(va_list args, state_args *state) {
  state_result res;
  char c = *(state->fs);
  switch (c) {
    case 'd':
      state->int_array = va_arg(args, int *);
      res.printed_chars = print_int_array(state->int_array, va_arg(args, int), 10, digit);
      res.next_state = st_printf_init;
      return res;

    case 'u': case 'b': case 'o': case 'x': case 'X':
      state->unsigned_int_array = va_arg(args, unsigned int *);
      res.printed_chars = print_unsigned_int_array(state->unsigned_int_array, va_arg(args, int), 
                                                   c == 'u' ? 10 : c == 'b' ? 2 : c == 'o' ? 8 : 16, c == 'X' ? DIGIT : digit);
      res.next_state = st_printf_init;
      return res;

    case 's':
      state->string_array = va_arg(args, char **);
      res.printed_chars = print_string_array(state->string_array, va_arg(args, int));
      res.next_state = st_printf_init;
      return res;

    case 'c':
      state->char_array = va_arg(args, char *);
      res.printed_chars = print_char_array(state->char_array, va_arg(args, int));
      res.next_state = st_printf_init;
      return res;

    default:
      toy_printf("Unhandled format %%A%c...\n", c);
      exit(-1);
  }
}

state_result padding_handler(va_list args, state_args *state) {
  state_result res;
  switch (*(state->fs)) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      state->width = (10 * state->width) + (*(state->fs) - '0');
      res.next_state = st_printf_padding;
      return res;

    case 'd':
      res.printed_chars = print_padded_int(va_arg(args, int), state->width, state->side);
      state->width = 0;
      res.next_state = st_printf_init;
      return res;

    case 's':
      if (state->side != 'z') {
        res.printed_chars = print_padded_string(va_arg(args, char *), state->width, state->side);
        state->width = 0;
        res.next_state = st_printf_init;
        return res;
      }
    
    default:
      toy_printf("Unhandled format...\n");
      exit(-1);
  }
}

/* SUPPORTED:
 *   %b, %d, %u, %o, %x, %X -- 
 *     integers in binary, decimal, octal, hex, and HEX
 *   %s -- strings
 */

int toy_printf(char *fs, ...) {
  int printed_chars = 0;
  va_list args;
  state_args state_args;
  state_args.width = 0;
  
  state_result state;
  state.next_state = st_printf_init;
  state.printed_chars = 0;

  va_start(args, fs);

  for (state_args.fs = fs; *(state_args.fs) != '\0'; ++state_args.fs) {
    switch (state.next_state) {
      case st_printf_init:
        state = init_handler(args, &state_args);
        printed_chars += state.printed_chars;
        break;

      case st_printf_percent:
        state = percent_handler(args, &state_args);
        printed_chars += state.printed_chars;
        break;

      case st_printf_array:
        state = array_handler(args, &state_args);
        printed_chars += state.printed_chars;
        break;

      case st_printf_padding:
        state = padding_handler(args, &state_args);
        printed_chars += state.printed_chars;
        break;

      default:
        toy_printf("toy_printf: Unknown state -- %d\n", (int)state.next_state);
        exit(-1);
    }
  }

  va_end(args);

  return printed_chars;
}
