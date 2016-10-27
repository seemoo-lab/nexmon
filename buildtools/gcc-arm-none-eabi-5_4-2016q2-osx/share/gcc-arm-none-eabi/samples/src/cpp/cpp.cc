#include <stdio.h>

struct test_class {
  test_class () { 
    m = new char[10];
    printf("In ctor\n");
  }
  ~test_class () {
    delete m;
    printf("In dtor\n");
  }
  char * m;
} g;

int main()
{
	printf("In main\n");
	return 0;
}
