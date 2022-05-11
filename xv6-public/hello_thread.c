#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
  printf(1, "Hello, thread!\n");
  int *ptr, idx;
  idx = 15000;
  ptr = (int*)malloc(65536);
  ptr[idx] = 777;
  printf(1, "ptr[%d] = %d\n", idx, ptr[idx]);
  free(ptr);
  exit();
}
