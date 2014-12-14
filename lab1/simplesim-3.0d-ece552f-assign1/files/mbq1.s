	.file	1 "mbq1.c"

 # GNU C 2.7.2.3 [AL 1.1, MM 40, tma 0.1] SimpleScalar running sstrix compiled by GNU C

 # Cc1 defaults:
 # -mgas -mgpOPT

 # Cc1 arguments (-G value = 8, Cpu = default, ISA = 1):
 # -quiet -dumpbase -O0 -o

gcc2_compiled.:
__gnu_compiled_c:
	.rdata
	.align	2
$LC0:
	.ascii	"Printing out the value of a: %d\n\000"
	.text
	.align	2
	.globl	main

	.text

	.loc	1 60
	.ent	main
main:
	.frame	$fp,40,$31		# vars= 16, regs= 2/0, args= 16, extra= 0
	.mask	0xc0000000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,40
	sw	$31,36($sp)
	sw	$fp,32($sp)
	move	$fp,$sp
	jal	__main
	li	$2,0x00000002		# 2
	sw	$2,16($fp)
	li	$2,0x00000001		# 1
	sw	$2,20($fp)
	li	$2,0x00000001		# 1
	sw	$2,24($fp)
 #APP
	li $2,0;li $3,30000;LOOP1:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP1;li $3,30000;LOOP2:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP2;li $3,30000;LOOP3:;addi $2,$2,1;addi $3,$3,-1;bgez $3,LOOP3;li $4,0;li $3,30000;LOOP4:;addi $2,$2,1;addi $3,$3,-1;addi $4,$4,0;bgez $3,LOOP4;li $3,30000;LOOP5:;addi $2,$2,1;addi $3,$3,-1;addi $4,$4,0;bgez $3,LOOP5;sw $2,16($fp);
 #NO_APP
	la	$4,$LC0
	lw	$5,16($fp)
	jal	printf
	move	$2,$0
	j	$L1
$L1:
	move	$sp,$fp			# sp not trusted here
	lw	$31,36($sp)
	lw	$fp,32($sp)
	addu	$sp,$sp,40
	j	$31
	.end	main
