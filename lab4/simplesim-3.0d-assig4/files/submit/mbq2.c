/* This is the microbenchmark for the stride prefetcher.
 * This mbq performs a positive test and a negative test.
 * For the positive test, the loop accesses a loop element
 * through predictable delta amount. The stride prefetcher performs
 * well here because the stride pattern is constant.
 *
 * The next for loop performs a negative testing. The 
 * stride prefetcher does not perform well here because
 * we are randomizing (pseudo-random) our stride pattern
 * so are table cannot arrive at a stable state.
 */
#include <stdio.h>
#define SIZE 1000000
#define BSIZE 64
int main (void) {
  int a[SIZE];
  int sum = 0;
  int i;

  // positive testing: the stride prefetcher does do well when there is a constant stride pattern
  for (i = 0; i < SIZE; i += 2*BSIZE) {
    sum += a[i];
  }

  // negative testing: the stride prefetcher does not do well when there is a randomized stride pattern
  while (i < SIZE) {
    if (i % 5 == 0) {
      i += 5*SIZE;
      if (i < SIZE) {
        sum += a[i];
        continue;
      }
    }
    if (i % 7 == 0) {
      i += 7*SIZE;
      if (i < SIZE) {
        sum += a[i];
        continue;
      }
    }
    if (i % 11 == 0) {
      i += 11*SIZE;
      if (i < SIZE) {
        sum += a[i];
        continue;
      }
    }
    if (i % 2 == 0) {
      i += 2*SIZE;
      if (i < SIZE) {
        sum += a[i];
        continue;
      }
    }
  }
  return 0;
}
