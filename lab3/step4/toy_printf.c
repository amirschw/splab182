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

void initialize_handlers();
int toy_printf(char *fs, ...);

state_result (*init_handlers[128])(char c);
state_result (*percent_handlers[128])(va_list args, state_args *state);
state_result (*array_handlers[128])(va_list args, state_args *state);
state_result (*padding_handlers[128])(va_list args, state_args *state);

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
    default:;
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
    default:;
  }

  return res;
}

state_result init_percent_handler(char c) {
  return (state_result){.printed_chars = 0, .next_state = st_printf_percent};
}

state_result init_default_handler(char c) {
  return (state_result){.printed_chars = print_char(c), .next_state = st_printf_init};
}

state_result init_handler(va_list args, state_args *state) {
  return init_handlers[(int)*(state->fs)](*(state->fs));
}

state_result percent_percent_handler(va_list args, state_args *state) {
  return (state_result){.printed_chars = print_char('%'), .next_state = st_printf_init};
}

state_result percent_int_handler(long n, int radix, const char *digit) {
  return (state_result){.printed_chars = print_int(n, radix, digit), .next_state = st_printf_init};
}

state_result percent_d_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, int), 10, digit);
}

state_result percent_u_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, unsigned int), 10, digit);
}

state_result percent_b_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, unsigned int), 2, digit);
}

state_result percent_o_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, unsigned int), 8, digit);
}

state_result percent_x_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, unsigned int), 16, digit);
}

state_result percent_X_handler(va_list args, state_args *state) {
  return percent_int_handler(va_arg(args, unsigned int), 16, DIGIT);
}

state_result percent_s_handler(va_list args, state_args *state) {
  return (state_result){.printed_chars = print_string(va_arg(args, char *)), .next_state = st_printf_init};
}

state_result percent_c_handler(va_list args, state_args *state) {
  return (state_result){.printed_chars = print_char((char)va_arg(args, int)), .next_state = st_printf_init};
}

state_result percent_A_handler(va_list args, state_args *state) {
  return (state_result){.printed_chars = 0, .next_state = st_printf_array};
}

state_result percent_dash_handler(va_list args, state_args *state) {
  state->side = 'l';
  return (state_result){.printed_chars = 0, .next_state = st_printf_padding};
}

state_result percent_0_handler(va_list args, state_args *state) {
  state->side = 'z';
  return (state_result){.printed_chars = 0, .next_state = st_printf_padding};
}

state_result percent_dec_digit_handler(va_list args, state_args *state) {
  state->width = *(state->fs) - '0';
  state->side = 'r';
  return (state_result){.printed_chars = 0, .next_state = st_printf_padding};
}

state_result percent_unknown_handler(va_list args, state_args *state) {
  toy_printf("Unhandled format %%%c...\n", *(state->fs));
  exit(-1);
}

state_result percent_handler(va_list args, state_args *state) {
  return percent_handlers[(int)*(state->fs)](args, state);
}

state_result array_d_handler(va_list args, state_args *state) {
  int *arr = va_arg(args, int *);
  return (state_result){.printed_chars = print_int_array(arr, va_arg(args, int), 10, digit), .next_state = st_printf_init};
}

state_result array_unsigned_handler(va_list args, state_args *state, int radix, const char *digit) {
  unsigned int *arr = va_arg(args, unsigned int *);
  return (state_result){.printed_chars = print_unsigned_int_array(arr, va_arg(args, int), radix, digit), .next_state = st_printf_init};
}

state_result array_u_handler(va_list args, state_args *state) {
  return array_unsigned_handler(args, state, 10, digit);
}

state_result array_b_handler(va_list args, state_args *state) {
  return array_unsigned_handler(args, state, 2, digit);
}

state_result array_o_handler(va_list args, state_args *state) {
  return array_unsigned_handler(args, state, 8, digit);
}

state_result array_x_handler(va_list args, state_args *state) {
  return array_unsigned_handler(args, state, 16, digit);
}

state_result array_X_handler(va_list args, state_args *state) {
  return array_unsigned_handler(args, state, 16, DIGIT);
}

state_result array_s_handler(va_list args, state_args *state) {
  char **arr = va_arg(args, char **);
  return (state_result){.printed_chars = print_string_array(arr, va_arg(args, int)), .next_state = st_printf_init};
}

state_result array_c_handler(va_list args, state_args *state) {
  char *arr = va_arg(args, char *);
  return (state_result){.printed_chars = print_char_array(arr, va_arg(args, int)), .next_state = st_printf_init};
}

state_result array_unknown_handler(va_list args, state_args *state) {
  toy_printf("Unhandled format %%A%c...\n", *(state->fs));
  exit(-1);
}

state_result array_handler(va_list args, state_args *state) {
  return array_handlers[(int)*(state->fs)](args, state);
}

state_result padding_dec_digit_handler(va_list args, state_args *state) {
  state->width = (10 * state->width) + (*(state->fs) - '0');
  return (state_result){.printed_chars = 0, .next_state = st_printf_padding};
}

state_result padding_d_handler(va_list args, state_args *state) {
  int printed = print_padded_int(va_arg(args, int), state->width, state->side);
  state->width = 0;
  return (state_result){.printed_chars = printed, .next_state = st_printf_init};
}

state_result padding_s_handler(va_list args, state_args *state) {
  if (state->side == 'z') {
    toy_printf("Unhandled format...\n");
    exit(-1);
  }
  int printed = print_padded_string(va_arg(args, char *), state->width, state->side);
  state->width = 0;
  return (state_result){.printed_chars = printed, .next_state = st_printf_init};
}

state_result padding_unknown_handler(va_list args, state_args *state) {
  toy_printf("Unhandled format...\n");
  exit(-1);
}

state_result padding_handler(va_list args, state_args *state) {
  return padding_handlers[(int)*(state->fs)](args, state);
}

state_result (*state_handlers[4])(va_list args, state_args *state) =
{init_handler, percent_handler, array_handler, padding_handler};

int toy_printf(char *fs, ...) {
  initialize_handlers();

  int printed_chars = 0;
  va_list args;
  state_args state_args;
  state_args.width = 0;
  
  state_result state;
  state.next_state = st_printf_init;
  state.printed_chars = 0;

  va_start(args, fs);

  for (state_args.fs = fs; *(state_args.fs) != '\0'; ++state_args.fs) {
    state = state_handlers[state.next_state](args, &state_args);
    printed_chars += state.printed_chars;
  }

  va_end(args);

  return printed_chars;
}


void initialize_handlers() {
  for (int i = 0; i < 128; i++) {
    if (i == '%') {
      percent_handlers[i] = percent_percent_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_percent_handler;
    } else if (i == 'd') {
      percent_handlers[i] = percent_d_handler;
      padding_handlers[i] = padding_d_handler;
      array_handlers[i] = array_d_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'u') {
      percent_handlers[i] = percent_u_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_u_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'b') {
      percent_handlers[i] = percent_b_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_b_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'o') {
      percent_handlers[i] = percent_o_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_o_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'x') {
      percent_handlers[i] = percent_x_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_x_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'X') {
      percent_handlers[i] = percent_X_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_X_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 's') {
      percent_handlers[i] = percent_s_handler;
      padding_handlers[i] = padding_s_handler;
      array_handlers[i] = array_s_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'c') {
      percent_handlers[i] = percent_c_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_c_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == 'A') {
      percent_handlers[i] = percent_A_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == '-') {
      percent_handlers[i] = percent_dash_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_default_handler;
    } else if (i == '0') {
      percent_handlers[i] = percent_0_handler;
      padding_handlers[i] = padding_dec_digit_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_default_handler;
    } else if (i >= '1' && i <= '9') {
      percent_handlers[i] = percent_dec_digit_handler;
      padding_handlers[i] = padding_dec_digit_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_default_handler;
    } else {
      percent_handlers[i] = percent_unknown_handler;
      padding_handlers[i] = padding_unknown_handler;
      array_handlers[i] = array_unknown_handler;
      init_handlers[i] = init_default_handler;
    }
  }
}

