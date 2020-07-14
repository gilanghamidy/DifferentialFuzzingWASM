typedef int (*func)(int, int);

int compute(func fPtr, int a, int b) {
  return fPtr(a, b);
}

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int div(int a, int b) { return a / b; }

func getfunc(int idx) {
  if(idx % 2) return &add;
  else if(idx % 3) return &sub;
  else if(idx % 5) return &mul;
  else return &div;
}