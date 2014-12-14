/* In this microbenchmark, we inlined assembly code to directly test the
 * functionality of our sim-safe.c code. To test the two stall RAW hazards,
 * we have three loops each with 30000 iterations. Each loop emulates a
 * RAW hazard (we add to $3 and $3 is used immediately afterwards to test
 * for branch condition). Since the loop runs three times, we would expect
 * a total number of 3*30000 = 90000 two stall hazards.
 *
 * Similarly, to test the one stall RAW hazards, we have two loops each
 * with 30000 iterations. Each loop emulates a one stall RAW hazard (add
 * to $3, execute a non-dependent instruction, use $3). Since the loop runs
 * two times, we would expect a total number of 2*30000 = 90000 one stall
 * hazards.
 *
 * Note that the number of loops in the two stalls test case is different
 * from the number of loops in the one stall test case. This is so we can
 * tell the different between the two.
 *
 * Also note that we created the intermediate assembly code of this program
 * using objdump and analyzed it. The resulting assembly code is dominated
 * by our inlined assembly code and thus, our test instructions should
 * dominant and any initialization effects can discounted. Here is a snippet
 * of the generated assembly:
 *
 *   main:
 *     .frame	$fp,40,$31		# vars= 16, regs= 2/0, args= 16, extra= 0
 *     .mask	0xc0000000,-4
 *     .fmask	0x00000000,0
 *     subu	$sp,$sp,40
 *     sw	$31,36($sp)
 *     sw	$fp,32($sp)
 *     move	$fp,$sp
 *     jal	__main
 *     li	$2,0x00000002		# 2
 *     sw	$2,16($fp)
 *     li	$2,0x00000001		# 1
 *     sw	$2,20($fp)
 *     li	$2,0x00000001		# 1
 *     sw	$2,24($fp)
 *    #APP
 *     li $2,0;li $3,30000;LOOP1:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP1;li $3,30000;LOOP2:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP2;li $3,30000;LOOP3:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP3;li $4,0;li $3,30000;LOOP4:;addi $2,$2,1;addi $3,$3,-1;addi $4,$4,0;bgez $3,LOOP4;li $3,30000;LOOP5:;addi $2,$2,1;addi $3,$3,-1;addi $4,$4,0;bgez $3,LOOP5;sw $2,16($fp);
 *    #NO_APP
 *     la	$4,$LC0
 *     lw	$5,16($fp)
 *     jal	printf
 *     move	$2,$0
 *     j	$L1
 *   $L1:
 *     move	$sp,$fp			# sp not trusted here
 *     lw	$31,36($sp)
 *     lw	$fp,32($sp)
 *     addu	$sp,$sp,40
 *     j	$31
 *     .end	main
 * 
 * Note that main() only has 30 lines of code. Thus our loop, located
 * on line 40 of the snippet (right after #APP) dominates over the
 * noise of the program.
 */
#include <stdio.h>
int main (void) {
  int a;
  int b;
  int i;
  a = 2;
  b = 1;
  i = 1;
  __asm__ __volatile__ (
                        // first three loops generate a two stalls.
                        "li $2,0;"
                        "li $3,30000;"
                        "LOOP1:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "bgez $3,LOOP1;"
                        "li $3,30000;"
                        "LOOP2:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "bgez $3,LOOP2;"
                        "li $3,30000;"
                        "LOOP3:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "bgez $3,LOOP3;"

                        // second two loops generate a one stall.
                        "li $4,0;"
                        "li $3,30000;"
                        "LOOP4:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "addi $4,$4,0;" // add this line to generate a 1 stall.
                        "bgez $3,LOOP4;"
                        "li $3,30000;"
                        "LOOP5:;"
                        "addi $2,$2,1;"
                        "addi $3,$3,-1;"
                        "addi $4,$4,0;" // add this line to generate a 1 stall.
                        "bgez $3,LOOP5;"

                        "sw $2,16($fp);"
      );
  printf("Printing out the value of a: %d\n",a);
  return 0;
}
