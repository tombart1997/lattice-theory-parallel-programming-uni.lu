int a;
int b;

void main() { 
  /*!npk a between 0 and 10 */
  a = 5;
  b = 0;
  while(a > 0)
  {
     b = b + 1;
  }
  assert(b < 10);
}
