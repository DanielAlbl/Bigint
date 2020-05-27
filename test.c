#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <bigint.h>

#define NTESTS 25000

char *filename="regression_tests.dat";

void matherr(bigint a, bigint b, bigint c, bigint d,char *msg)
{
  char *s1,*s2,*s3,*s4;
  s1 = bigint_to_str(a);
  s2 = bigint_to_str(b);
  s3 = bigint_to_str(c);
  s4 = bigint_to_str(d);
  printf("Error in %s:\ninputs:\n%s\n%s\n",msg,s1,s2);
  printf("correct output: %s\n",s3);
  printf("output: %s\n",s4);
  exit(4);
}

void sqrterr(bigint a, bigint c, bigint d,char *msg)
{
  char *s1,*s3,*s4;
  s1 = bigint_to_str(a);
  s3 = bigint_to_str(c);
  s4 = bigint_to_str(d);
  printf("Error in %s:\ninput: %s\n",msg,s1);
  printf("correct output: %s\n",s3);
  printf("output: %s\n",s4);
  exit(4);
}

int main()
{
  static bigint a[NTESTS];
  static bigint b[NTESTS];
  static bigint addr[NTESTS];
  static bigint subr[NTESTS];
  static bigint mulr[NTESTS];
  static bigint sqrtr[NTESTS];
  static bigint divr[NTESTS];
  bigint d,aabs,babs;
  int i,N;
  FILE *inf;
  clock_t start,stop,total=0;
  
  printf("Reading Data\n");
  if((inf = fopen(filename,"r"))==NULL)
    {
      perror("Failed to open file");
      exit(2);
    }
  
  N = 0;
  do {
    a[N] = bigint_read_binary(inf);
    b[N] = bigint_read_binary(inf);
    addr[N] = bigint_read_binary(inf);
    subr[N] = bigint_read_binary(inf);
    mulr[N] = bigint_read_binary(inf);
    sqrtr[N] = bigint_read_binary(inf);
    divr[N] = bigint_read_binary(inf);
      N++;
  } while((N < NTESTS) && (a[N-1] != NULL) && (b[N-1] != NULL) && 
	  (addr[N-1] != NULL) && (subr[N-1] != NULL) &&
	  (mulr[N-1] != NULL) && (divr[N-1] != NULL));
  fclose(inf);

  printf("Testing Addition\n");
  start = clock();
  for(i=0;i<N;i++)
    {
      d = bigint_add(a[i],b[i]);   
      if(bigint_ne(addr[i],d))
	matherr(a[i],b[i],addr[i],d,"add");
      bigint_free(d);
    }
  stop = clock();
  printf("Addition : %lf\n",(stop-start)/(double)CLOCKS_PER_SEC);
  total += (stop-start);

  printf("Testing Subtraction\n");
  start = clock();
  for(i=0;i<N;i++)
    {
      d = bigint_sub(a[i],b[i]);   
      if(bigint_ne(subr[i],d))
	matherr(a[i],b[i],subr[i],d,"sub");
      bigint_free(d);
    }
  stop = clock();
  printf("Subtraction : %lf\n",(stop-start)/(double)CLOCKS_PER_SEC);
  total += (stop-start);

  printf("Testing Multiplication\n");
  start = clock();
  for(i=0;i<N;i++)
    {
      d = bigint_mul(a[i],b[i]);   
      if(bigint_ne(mulr[i],d))
	matherr(a[i],b[i],mulr[i],d,"mul");
      bigint_free(d);
    }
  stop = clock();
  printf("Multiplication : %lf\n",(stop-start)/(double)CLOCKS_PER_SEC);
  total += (stop-start);

  printf("Testing Division\n");
  start = clock();
  for(i=0;i<N;i++)
    {
      /* Use largest absolute value for dividend and smaller absoulute
	 value as divisor */
      aabs = bigint_abs(a[i]);
      babs = bigint_abs(b[i]);
      if(bigint_gt(aabs,babs))
	d = bigint_div(a[i],b[i]);
      else
	d = bigint_div(b[i],a[i]);
      if(bigint_ne(divr[i],d))
	matherr(a[i],b[i],divr[i],d,"div");
      bigint_free(aabs);
      bigint_free(babs);
      bigint_free(d);
    }
  stop = clock();
  printf("Division : %lf\n",(stop-start)/(double)CLOCKS_PER_SEC);
  total += (stop-start);

  printf("Testing Square Root\n");
  start = clock();
  for(i=0;i<N;i++)
    {
      aabs = bigint_abs(a[i]);
      d = bigint_sqrt(aabs);
      if(bigint_ne(sqrtr[i],d))
	sqrterr(aabs,sqrtr[i],d,"sqrt");
      bigint_free(aabs);
      bigint_free(d);
    }
  stop = clock();
  printf("Square Root : %lf\n",(stop-start)/(double)CLOCKS_PER_SEC);
  total += (stop-start);

  printf("Total time : %lf\n",(total)/(double)CLOCKS_PER_SEC);

  for(i=0;i<N;i++)
    {
      bigint_free(a[i]);
      bigint_free(b[i]);
      bigint_free(addr[i]);
      bigint_free(subr[i]);
      bigint_free(mulr[i]);
      bigint_free(sqrtr[i]);
      bigint_free(divr[i]);
    }
  return 0;
}
