#include <stdio.h>

void main (){

int a = 0;
int c = 12;
int d = 5;
int e = 0;
int i = 1;

while ( i < 50000000){
  a = c + i;
  e = a + i;
  c = a + d;
  d = c + a;
  i++;
}
}
