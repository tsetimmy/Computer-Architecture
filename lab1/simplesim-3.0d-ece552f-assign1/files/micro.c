#include <stdio.h>
int main (void) {
  int a;
  int b;
  int i;
  a = 2;
  b = 1;
  i = 1;
  __asm__ __volatile__ (
                        "li $2,0;"
                        "li $4,0;"

                        // immediate RAW - add 30000 one stalls
                        "li $3,30000;"
                        "LOOP1:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "bgez $3,LOOP1;"

                        // non-immediate RAW - adds nothing
                        "li $3,30000;"
                        "LOOP2:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "addi $4,$4,0;"
                        "bgez $3,LOOP2;"

                        // immediate LTU - add 30000 two stalls
                        "li $3,30000;"
                        "LOOP3:;"
                        "lw $2,16($fp);"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "addi $4,$4,0;"
                        "bgez $3,LOOP3;"

                        // non-immediate LTU - add 30000 one stalls
                        "li $3,30000;"
                        "LOOP4:;"
                        "lw $2,16($fp);"
                        "addi $4,$4,0;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "addi $4,$4,0;"
                        "bgez $3,LOOP4;"

                        // end
                        "sw $2,16($fp);"
      );

  printf("Printing out the value of a: %d\n",a);
  return 0;

}
