#include <stdio.h>

#define YY_INPUT(buf, result, max, D)			\
{							\
  int c= getchar();					\
  result= (EOF == c) ? 0 : (*(buf)= c, 1);		\
  if (EOF != c) printf("<%c>\n", c);			\
}

#include "left.peg.c"

int main()
{
  GREG g;
  yyinit(&g);
  printf(yyparse(&g) ? "success\n" : "failure\n");
  yydeinit(&g);

  return 0;
}
