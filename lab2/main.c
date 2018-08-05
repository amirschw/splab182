#include "toy_stdio.h"


int main(int argc, char *argv[]) {
  toy_printf("Non-padded string: %s\n", "str");
  toy_printf("Right-padded string: %6s\n", "str");
  toy_printf("Left-added string: %-6s\n", "str");
  toy_printf("With numeric placeholders: %05d\n", -1);
}