/* This is the microbenchmark for the open ended prefetcher.
 * We used a combination of code from both the next line
 * prefetcher and the stride prefetcher. We hoped that
 * in combination with a stride prefetcher component
 * and a history prediction component, it will be
 * able to predict accurately for both cases when there
 * is a constant stride and when there is not a constant
 * stride.
 */
#include <stdio.h>
#define SIZE 1000000
#define BSIZE 64
int main (void) {
  int a[SIZE];
  int sum = 0;

  int i;
  for (i = 0; i < SIZE; i++) {
    a[i] = i;
  }

  for (i = 0; i < SIZE; i += 2*BSIZE) {
    sum += a[i];
  }

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
