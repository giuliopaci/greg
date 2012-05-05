/* Copyright (c) 2007 by Ian Piumarta
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 * 
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 * 
 * Last edited: 2007-09-12 00:27:30 by piumarta on vps2.piumarta.com
 */

#include "greg.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>

FILE *input= 0;

int   verboseFlag= 0;

static int   lineNumber= 0;
static char *fileName= 0;

struct _GREG *G;
void yyerror(struct _GREG *G, char *message);

#define YY_INPUT(buf, result, max, D)			\
{							\
  int c= getc(input);					\
  if ('\n' == c || '\r' == c) ++lineNumber;		\
  result= (EOF == c) ? 0 : (*(buf)= c, 1);		\
}

#define YY_LOCAL(T)	static T
#define YY_RULE(T)	static T

#include "rpeg.peg-c"

void yyerror(struct _GREG *G, char *message)
{
  fprintf(stderr, "%s:%d: %s", fileName, lineNumber, message);
  if (G->text[0]) fprintf(stderr, " near token '%s'", G->text);
  if (G->pos < G->limit || !feof(input))
    {
      G->buf[G->limit]= '\0';
      fprintf(stderr, " before text \"");
      while (G->pos < G->limit)
	{
	  if ('\n' == G->buf[G->pos] || '\r' == G->buf[G->pos]) break;
	  fputc(G->buf[G->pos++], stderr);
	}
      if (G->pos == G->limit)
	{
	  int c;
	  while (EOF != (c= fgetc(input)) && '\n' != c && '\r' != c)
	    fputc(c, stderr);
	}
      fputc('\"', stderr);
    }
  fprintf(stderr, "\n");
  exit(1);
}

static void version(char *name)
{
  printf("%s version %d.%d.%d\n", name, GREG_MAJOR, GREG_MINOR, GREG_LEVEL);
}

static void usage(char *name)
{
  version(name);
  fprintf(stderr, "usage: %s [<option>...] [<file>...]\n", name);
  fprintf(stderr, "where <option> can be\n");
  fprintf(stderr, "  -h          print this help information\n");
  fprintf(stderr, "  -o <ofile>  write output to <ofile>\n");
  fprintf(stderr, "  -v          be verbose\n");
  fprintf(stderr, "  -V          print version number and exit\n");
  fprintf(stderr, "if no <file> is given, input is read from stdin\n");
  fprintf(stderr, "if no <ofile> is given, output is written to stdout\n");
  exit(1);
}

int main(int argc, char **argv)
{
  GREG *G;
  Node *n;
  int   c;

  output= stdout;
  input= stdin;
  lineNumber= 1;
  fileName= "<stdin>";

  while (-1 != (c= getopt(argc, argv, "Vho:v")))
    {
      switch (c)
	{
	case 'V':
	  version(basename(argv[0]));
	  exit(0);

	case 'h':
	  usage(basename(argv[0]));
	  break;

	case 'o':
	  if (!(output= fopen(optarg, "w")))
	    {
	      perror(optarg);
	      exit(1);
	    }
	  break;

	case 'v':
	  verboseFlag= 1;
	  break;

	default:
	  fprintf(stderr, "for usage try: %s -h\n", argv[0]);
	  exit(1);
	}
    }
  argc -= optind;
  argv += optind;

  G = yyparse_new(NULL);
  if (argc)
    {
      for (;  argc;  --argc, ++argv)
	{
	  if (!strcmp(*argv, "-"))
	    {
	      input= stdin;
	      fileName= "<stdin>";
	    }
	  else
	    {
	      if (!(input= fopen(*argv, "r")))
		{
		  perror(*argv);
		  exit(1);
		}
	      fileName= *argv;
	    }
	  lineNumber= 1;
	  if (!yyparse(G))
	    yyerror(G, "syntax error");
	  if (input != stdin)
	    fclose(input);
	}
    }
  else
    if (!yyparse(G))
      yyerror(G, "syntax error");
  yyparse_free(G);

  if (verboseFlag)
    for (n= rules;  n;  n= n->any.next)
      Rule_print(n);

  Rule_compile_c_header();
  if (rules) Rule_compile_c(rules);

  return 0;
}
