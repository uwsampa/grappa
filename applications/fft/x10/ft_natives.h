#include <stdint.h>

extern "C" {
void execute_plan(int64_t, double*, double*, signed int, signed int, signed int);
int64_t create_plan(signed int, signed int, signed int);
}
