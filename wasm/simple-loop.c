#include <stdint.h>
#include <stdio.h>
extern int rand();

int __attribute__ ((visibility ("default"))) func_caller(int looper) {
  int res = 3;

  if(looper % 5 != 0) {
    looper *= rand();
  }
  res *= looper;
  return res;
}

int __attribute__ ((visibility ("default"))) do_while(int a, int b) {
  int res = 3;
  do {
    ++a;
    res *= b;
  } while(a <= 100);
  res *= a;
  return res;
}

int __attribute__ ((visibility ("default"))) while_loop(int a, int b) {
  int res = 3;
  while(a <= 100) {
    a++;
    if(a * b == 100)
      break;
    res *= b;
  }
  res *= rand();
  return res * a;
}

int __attribute__ ((visibility ("default"))) for_loop(int a, int b) {
  int res = 3;
  for(int i = a; i <= 100; i++) {
    a = i;
    res *= b;
  }
  res *= a;
  return res;
}


int main() {
  printf("Hello, World!\n");
  printf("Enter 2 numbers:\n");
  int a, b;
  scanf("%d", &a);
  scanf("%d", &b);
  int res = for_loop(a, b);
  printf("Computing for_loop(%d, %d): %d\n", a, b, res);
  return 0;
}
