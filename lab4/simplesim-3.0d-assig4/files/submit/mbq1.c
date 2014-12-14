/* This is the microbenchmark for the next line prefetcher.
 * This mbq performs a positive test and a negative test.
 * For the positive test, the loop accesses a loop element
 * by element sequentially. The next line prefetcher performs
 * well here because instructions are executed line by line
 * and thus, each line brought by the line is a hit (except
 * for the cold misses in the beginning).
 *
 * The next for loop performs a negative testing. The next
 * line prefetcher does not perform well here because
 * on every access, BSIZE amount of bytes get prefetched but
 * we skip 2*BSIZE ahead for each access. Essentially,
 * we are skipping over all of the prefetched data.
 */
#include <stdio.h>
#define SIZE 1000000
#define BSIZE 64
int main (void) {
  int a[SIZE];
  int sum = 0;

  // positive testing: the next line prefetcher should do well in a sequential loop access
  int i;
  for (i = 0; i < SIZE; i++) {
    a[i] = i;
  }

  // negatives testing: the next line prefectcher will do poorly when accessing 2 * BSIZE every iteration
  for (i = 0; i < SIZE; i += 2*BSIZE) {
    sum += a[i];
  }
  return 0;
}
