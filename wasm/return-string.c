char const* get_text(int v) {
  if(v % 2) return "it is an even number";
  else if(v % 3) return "it is divisible by three";
  else if(v % 5) return "it is divisible by five";
  else return "it is some odd number";
}