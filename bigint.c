#include <bigint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#ifdef EIGHT_BIT
typedef uint8_t chunk;
typedef int8_t schunk;
typedef uint16_t bigchunk;
#define CHUNKMASK 0xFFU
#else
#ifdef SIXTEEN_BIT
typedef uint16_t chunk;
typedef int16_t schunk;
typedef uint32_t bigchunk;
#define CHUNKMASK 0xFFFFU
#else
typedef uint32_t chunk;
typedef int32_t schunk;
typedef uint64_t bigchunk;
#define CHUNKMASK 0xFFFFFFFFU
#endif
#endif

#define BITSPERCHUNK ((sizeof(chunk)<<3))

/* A bigint is an array of chunks of bits */
struct bigint_struct{
  chunk *blks;        /* array of bit chunks */
  int size;           /* number of chunks in the array */
};

#define MAX(a,b) ((a<b)?b:a)

bigint bigint_adc(bigint l, bigint r, chunk carry);

/********************************************************/
/*  Utility functions                                   */
/********************************************************/
void alloc_err()
{
  printf("error allocating\n");
  exit(1);
}

bigint bigint_alloc(int chunks)
{
  bigint r;
  if((r = (bigint)malloc(sizeof(struct bigint_struct))) == NULL)
    {
      perror("bigint_alloc");
      exit(1);
    }
  r->size = chunks;
  r->blks = (chunk*)malloc(chunks * sizeof(chunk));
  if(r->blks == NULL)
    {
      perror("bigint_alloc");
      exit(1);
    }
  return r;
}

/********************************************************/
void bigint_free(bigint b)
{
  if(b != NULL)
    {
      if(b->blks != NULL)
	free(b->blks);
      free(b);
    }
}

/********************************************************/
void bigint_dump(bigint b)
{
  int i;
  printf("%d chunks:",b->size);
  for(i=b->size-1;i>=0;i--)
    printf(" %02X",b->blks[i]);
  printf("\n");
}

/********************************************************/
bigint bigint_trim(bigint b)
{
  bigint d;
  int i = b->size-1;
  if(i>0)
    {
      if(b->blks[i] == 0)
	{ // we have a leading block that is all 0
	  do
	    i--; // search for first block that is not all 0
	  while ((i>0) && (b->blks[i]==0));
	  if(b->blks[i] & (1<<(BITSPERCHUNK-1)))
	    i++;  // if msb of current block is 1, then we went too far
	}
      else
	if(b->blks[i] == CHUNKMASK)
	  { // we have a leading block that is all 1
	    do
	      i--; // search for first block that is not all 1
	    while ((i>0) && (b->blks[i]==CHUNKMASK));
	    if(!(b->blks[i] & (1<<(BITSPERCHUNK-1))))
	      i++;  // if msb of current block is 0, then we went too far
	  }
    }
  i++;  // i is now the number of blocks to copy
  if(i < b->size)
    {
      d = bigint_alloc(i);
      memcpy(d->blks,b->blks,d->size*sizeof(chunk));
    }
  else
    d = bigint_copy(b);
  return d;
}

/********************************************************/
/* smallmod divides a bigint by a small number
   and returns the modulus. b changes as a SIDE-EFFECT.  
   This is used by the to_str function. */
unsigned bigint_smallmod(bigint b,chunk num)
{
  bigchunk tmp;
  int i;
  if(num >= (1<<(BITSPERCHUNK-1)))
    {
      fprintf(stderr,"bigint_smallmod: divisor out of range\n");
      exit(1);
    }
  /* start with most significant chunk and work down, taking
     two overlapping chunks at a time */
  tmp = b->blks[b->size-1];
  for(i=b->size-1; i>0;i--)
    {
      b->blks[i] = tmp/num;      
      tmp = ((tmp % num) << BITSPERCHUNK) | b->blks[i-1];
    }
  b->blks[0] = tmp/num;
  tmp = (tmp % num);
  return tmp;
}

/********************************************************/
/* bigint_cmp compares two bigints 
   returns -1 if l<r
   returns 0 if l==r
   returns 1 if l>r
*/
int bigint_cmp(bigint l, bigint r)
{
  int cmp;
  bigint d = bigint_sub(l,r);
  if((d->size == 0) || (d->size == 1 && d->blks[0] == 0))
    cmp = 0;
  else
    if(d->blks[d->size-1] & (1<<(BITSPERCHUNK-1)))
      cmp=-1;
    else
      cmp=1;
  bigint_free(d);
  return cmp;
}

/********************************************************/
inline int bigint_is_zero(bigint b)
{
  int i;
  for(i=0;i<b->size;i++)
    if(b->blks[i])
      return 0;
  return 1;
}

/********************************************************/
bigint bigint_shift_left_chunk(bigint l, int chunks)
{
   bigint tmp;
   int i;
   tmp=bigint_alloc(l->size+chunks);
   for(i=-chunks;i<l->size;i++)
     {
       if(i<0)
	 tmp->blks[i+chunks]=0;
       else
	 tmp->blks[i+chunks]=l->blks[i];
     }
   return tmp;
}

/********************************************************/
bigint bigint_shift_right_chunk(bigint l, int chunks)
{
   bigint tmp;
   int i;
   tmp=bigint_alloc(l->size-chunks);
   for(i=0;i<tmp->size;i++)
     {
       if(i<chunks)
	 tmp->blks[i]=0;
       else
	 tmp->blks[i]=l->blks[i-chunks];
     }
   return tmp;
}

/********************************************************/
/* Conversion and copy functions                        */
/********************************************************/
bigint bigint_copy(bigint source)
{
  bigint r;
  r = bigint_alloc(source->size);
  memcpy(r->blks,source->blks,r->size*sizeof(chunk));
  return r;
}

/********************************************************/
bigint bigint_complement(bigint b)
{
  int i;
  bigint r = bigint_copy(b);
  for(i=0;i<r->size;i++)
    r->blks[i] ^= CHUNKMASK;
  return r;
}

/********************************************************/
bigint bigint_negate(bigint b)
{
  bigint tmp1,tmp2;
  bigint r = bigint_complement(b);
  tmp1=bigint_from_int(0);
  tmp2=bigint_adc(r,tmp1,1);
  bigint_free(tmp1);
  bigint_free(r);
  return tmp2;
}

/********************************************************/
char *bigint_to_str(bigint b)
{
  int chars,i,negative=0;
  unsigned remainder;
  char *s,*r;
  bigint tmp,tmp2;
  /* rough estimate of the number of characters needed */
  chars = log10(pow(2.0,(b->size * BITSPERCHUNK)))+3;
  i = chars-1;
  if((s = (char*)malloc(1 + chars * sizeof(char))) == NULL)
    {
      perror("bigint_str");
      exit(1);
    }
  s[i]=0;
  tmp = bigint_copy(b);
  if(tmp->blks[tmp->size-1] & (1<< (BITSPERCHUNK-1)))
    {
      negative=1;
      tmp2 = bigint_negate(tmp);
      bigint_free(tmp);
      tmp=tmp2;
    }
  if(bigint_is_zero(tmp))
    s[--i] = '0';
  else
    do
      {
	remainder = bigint_smallmod(tmp,10);
	s[--i] = remainder + '0';
      } while(!bigint_is_zero(tmp));
  if(negative)
    s[--i] = '-';
  r = strdup(s+i);
  bigint_free(tmp);
  free(s);
  return r;
}

/********************************************************/
bigint bigint_from_str(char *s)
{
  bigint d;
  bigint power;
  bigint ten;
  bigint tmp;
  bigint currprod;
  int i,negative=0;
  d = bigint_from_int(0);
  ten =   bigint_from_int(10);
  power = bigint_from_int(1);
  if(*s == '-')
    {
      negative = 1;
      s++;
    }
  for(i=strlen(s)-1; i>=0;i--)
    {
      if(!isdigit(s[i]))
	{
	  fprintf(stderr,"Cannot convert string to bigint\n");
	  exit(1);
	}
      tmp = bigint_from_int(s[i]-'0');
      currprod = bigint_mul(tmp,power);
      bigint_free(tmp);
      tmp = bigint_adc(currprod,d,0);
      bigint_free(d);
      d=tmp;
      bigint_free(currprod);
      if(i>0)
	{
	  tmp = bigint_mul(power,ten);
	  bigint_free(power);
	  power = tmp;
	}
    }
  if(negative)
    {
      tmp=bigint_negate(d);
      bigint_free(d);
      d=tmp;
    }
  bigint_free(ten);
  bigint_free(power);
  return d;
}

/********************************************************/
int bigint_to_int(bigint b)
{
  int i,negative=0,result=0;
  bigint tmp1, tmp2;
  tmp1 = bigint_trim(b); /* make a trimmed copy */
  if(tmp1->size*sizeof(chunk) > sizeof(int))
    {
      fprintf(stderr,
	      "Cannot convert bigint to int\n%ld bytes\n",
	      (long)tmp1->size*sizeof(chunk));
      exit(1);
    }
  /* check sign and negate if necessary */
  if(tmp1->blks[tmp1->size-1] & (1<<(BITSPERCHUNK-1)))
    {
      negative=1;
      tmp2=bigint_negate(tmp1);
      bigint_free(tmp1);
      tmp1=tmp2;
    }
  for(i=tmp1->size-1;i>=0;i--)
    result |= (tmp1->blks[i]<<(i*BITSPERCHUNK));
  bigint_free(tmp1);
  if(negative)
    result = -result;
  return result;
}

/********************************************************/
bigint bigint_from_int(int val)
{
  bigint d,tmp;
  int i;
  int nchunks = sizeof(int)/sizeof(chunk);
  d = bigint_alloc(nchunks);
  for(i=0;i<d->size;i++)
    d->blks[i] = (val >> (i*BITSPERCHUNK)) & CHUNKMASK;
  tmp = bigint_trim(d);
  bigint_free(d);
  return tmp;
}

/********************************************************/
bigint  bigint_extend(bigint b,int nchunks)
{
  bigint tmp;
  int i,negative;
  negative=0;
  if(b->blks[b->size-1] & (1<<(BITSPERCHUNK-1)))
    negative=1;
  tmp = bigint_alloc(nchunks);
  for(i=0;i<nchunks;i++)
    if(i < b->size)
      tmp->blks[i] = b->blks[i];
    else
      if(negative)
	tmp->blks[i] = CHUNKMASK;
      else
	tmp->blks[i] = 0;
  return tmp;
}

#ifndef USE_ASM
/********************************************************/
/* this is the internal add function.  It includes a    */
/* carry. Several other functions use it.               */
bigint bigint_adc(bigint l, bigint r, chunk carry)
{
  bigint sum,tmpl,tmpr;
  int i,nchunks;
  bigchunk tmpsum;
  /* allocate one extra chunk to make sure overflow
     cannot occur */
  nchunks = MAX(l->size,r->size)+1;
  /* make sure both operands are the same size */
  tmpl = bigint_extend(l,nchunks);
  tmpr = bigint_extend(r,nchunks);
  /* allocate space for the result */
  sum = bigint_alloc(nchunks);
  /* perform the addition */
  for(i=0 ;i < nchunks ; i++)
    {
      /* add the current block of bits */
      tmpsum = (bigchunk)tmpl->blks[i] + tmpr->blks[i] + carry;
      sum->blks[i] = tmpsum & CHUNKMASK;
      /* calculate the carry bit for the next block */
      carry = (tmpsum >> BITSPERCHUNK)&CHUNKMASK;
    }
  bigint_free(tmpl);
  bigint_free(tmpr);
  tmpl = bigint_trim(sum);
  bigint_free(sum);
  return tmpl;
}
#endif

/********************************************************/
/* Mathematical operations                              */
/********************************************************/


/********************************************************/
/* The add function calls adc to perform an add with    */
/* initial carry of zero                                */
bigint bigint_abs(bigint b)
{
  bigint r;
  if(b->blks[b->size-1]&(1<<(BITSPERCHUNK-1)))
    r = bigint_negate(b);
  else
    r = bigint_copy(b);
  return r;
}

/********************************************************/
/* The sqrt function returns floor(sqrt(b))             */
/* initial carry of zero                                */
bigint bigint_sqrt(bigint b)
{
  bigint r,bit,zero,num,tmp,resplusbit;
  int i;
  r = bigint_from_int(0);
  zero = bigint_from_int(0);
  num = bigint_copy(b);
  if(b->blks[b->size-1]&(1<<(BITSPERCHUNK-1)))
    {
      fprintf(stderr,
	      "Cannot compute square root of negative number.\n");
      exit(1);
    }
  /* initialize bit to the largest power of 4 that is <= b */
  bit = bigint_alloc(b->size);
  bit->blks[bit->size-1] = (1<<(BITSPERCHUNK-2));
  for(i=0;i<bit->size-1;i++)
    bit->blks[i] = 0;
  while(bit->blks[bit->size-1] > b->blks[b->size-1])
    bit->blks[bit->size-1] >>= 2;
  if(bit->blks[bit->size-1] ==0)
    bit->blks[bit->size-2] = (1<<(BITSPERCHUNK-2));
  /* this could be more efficient */  
  while(bigint_gt(bit,zero)) 
    {
      resplusbit = bigint_add(r,bit);
      if(bigint_ge(num,resplusbit))
	{
	  tmp = num;
	  num = bigint_sub(num,resplusbit);
	  bigint_free(tmp);
	  tmp = bigint_shift_right(r,1);
	  bigint_free(r);
	  r = bigint_add(tmp,bit);
	  bigint_free(tmp);
	}
      else
	{
	  tmp = r;
	  r = bigint_shift_right(r,1);
	  bigint_free(tmp);
	}
      bigint_free(resplusbit);
      tmp = bit;
      bit = bigint_shift_right(bit,2);
      bigint_free(tmp);
    }
  bigint_free(zero);
  bigint_free(num);
  bigint_free(bit);
  return r;
}

/********************************************************/
/* The add function calls adc to perform an add with    */
/* initial carry of zero                                */
bigint bigint_add(bigint l, bigint r)
{
  return bigint_adc(l,r,0);
} 
    
/********************************************************/
bigint bigint_sub(bigint l, bigint r)
{
  bigint tmp1,tmp2;
  tmp1 = bigint_complement(r);
  tmp2 = bigint_adc(l,tmp1,1);
  bigint_free(tmp1);
  return tmp2;
}

/********************************************************/
bigint bigint_shift_left(bigint l, int shamt)
{
  int extra,i;
  bigint tmp;
  l =  bigint_extend(l,l->size+1);
  extra = shamt % BITSPERCHUNK;
  shamt = shamt / BITSPERCHUNK;
  if(shamt)
    {
      tmp = l;
      l =  bigint_shift_left_chunk(l,shamt);
      bigint_free(tmp);
    }
  if(extra)
    {
      for(i=l->size-1;i>0;i--)
	{
	  l->blks[i] = (l->blks[i]<<extra) |
	    (l->blks[i-1]>>(BITSPERCHUNK-extra));
	}
      l->blks[0] = (l->blks[0]<<extra);
    }
  tmp = bigint_trim(l);
  bigint_free(l);
  return tmp; 
}

/********************************************************/
bigint bigint_shift_right(bigint l, int shamt)
{
  int extra,i;
  bigint tmp;
  schunk tmpc;
  extra = shamt % BITSPERCHUNK;
  shamt = shamt / BITSPERCHUNK;
  l =  bigint_shift_right_chunk(l,shamt);
  if(extra)
    {
      for(i=0;i<l->size-1;i++)
	{
	  l->blks[i] = (l->blks[i]>>extra) |
	    (l->blks[i+1]<<(BITSPERCHUNK-extra));
	}
      /* do a signed shift of the top chunk */
      tmpc = l->blks[l->size-1];
      tmpc >>= extra;
      l->blks[l->size-1] = tmpc;
    }
  tmp = bigint_trim(l);
  bigint_free(l);
  return tmp; 
}

/********************************************************/
bigint bigint_mul_uint(bigint l, unsigned r)
{
  bigint sum;
  bigint tmp1,tmp2;
  int i,negative=0;
  bigchunk tmpchunk;
  sum = bigint_from_int(0);
  /* make sure the right operand is not too large */
  if(r > CHUNKMASK)
    {
      fprintf(stderr,"bigint_mul_uint: Integer too large\n");
      exit(1);
    }
  /* make sure the left operand is not negative */
  if(l->blks[l->size-1]&(1<<(BITSPERCHUNK-1)))
    {
      negative ^= 1;
      l = bigint_negate(l);
    }
  /* perform the multiply */
  for(i=0;i<l->size;i++)
    {
      tmpchunk = (bigchunk)l->blks[i] * r;
      tmp1 = bigint_alloc(3);
      tmp1->blks[0] = tmpchunk & CHUNKMASK;
      tmp1->blks[1] = (tmpchunk>>BITSPERCHUNK) & CHUNKMASK;
      tmp1->blks[2] = 0;
      tmp2 = bigint_shift_left_chunk(tmp1,i);
      bigint_free(tmp1);
      tmp1=bigint_adc(sum,tmp2,0);
      bigint_free(sum);
      bigint_free(tmp2);
      sum = tmp1;
    }
  /* result may need to be negated */
  if(negative)
    {
      tmp1 = sum;
      sum = bigint_negate(sum);
      bigint_free(tmp1);
      bigint_free(l);
    }
  return sum;
}

/********************************************************/
/* bigint_mul uses the algorithm from Section 7.2.5     */
bigint bigint_mul(bigint l, bigint r)
{
  bigint sum;
  bigint tmp1,tmp2;
  int i,negative=0;
  /* the result may require the sum
     of the number of chunks in l and r */
  sum = bigint_from_int(0);
  /* make sure the right operand is not negative */
  if(r->blks[r->size-1] & (1<<(BITSPERCHUNK-1)))
    {
      negative=1;
      r=bigint_negate(r); /* make negated copy of r */
    }
  for(i=0;i<r->size;i++)
    {
      tmp1 = bigint_mul_uint(l,r->blks[i]);
      tmp2 = bigint_shift_left_chunk(tmp1,i);
      bigint_free(tmp1);
      tmp1 = sum;
      sum = bigint_adc(sum,tmp2,0);
      bigint_free(tmp1);
      bigint_free(tmp2);
    }
  if(negative)
    {
      tmp1 = sum;     /* copy original */
      sum = bigint_negate(sum); /* create complement */
      bigint_free(tmp1); /* free original */
      bigint_free(r);
    }
  return sum;
}

/********************************************************/
bigint bigint_div(bigint l, bigint r)
{
  bigint lt,rt,tmp,q;
  int shift,chunkshift,negative=0;
  q = bigint_from_int(0);
  lt = bigint_trim(l);
  rt = bigint_trim(r);
  if(lt->size >= rt->size)
    {
      /* make sure the right operand is not negative */
      if(r->blks[r->size-1]&(1<<(BITSPERCHUNK-1)))
	{
	  negative = 1;  /* track sign of result */
	  tmp = rt;
	  rt = bigint_negate(rt);
	  bigint_free(tmp);
	}
      /* make sure the left operand is not negative */
      if(l->blks[l->size-1]&(1<<(BITSPERCHUNK-1)))
	{
	  negative ^= 1;  /* track sign of result */
	  tmp = lt;
	  lt = bigint_negate(lt);
	  bigint_free(tmp);
	}
      /* do shift by chunks */
      chunkshift = lt->size - rt->size;
      if(chunkshift>0)
	{
	  tmp = rt;
	  rt = bigint_shift_left_chunk(rt,chunkshift);
	  bigint_free(tmp);
	}
      /* do remaining shift bit-by-bit */
      shift = 0;
      while((shift < (BITSPERCHUNK-1)) && bigint_lt(rt,lt))
	{
	  shift++;
	  tmp = rt;
	  rt = bigint_shift_left(rt,1);
	  bigint_free(tmp);
	}
      shift += (chunkshift * BITSPERCHUNK); /* total shift */
      /* loop to shift right and subtract */
      while(shift >= 0)
	{
	  tmp = q;
	  q = bigint_shift_left(q,1);
	  bigint_free(tmp);
	  if(bigint_le(rt,lt))
	    {
	      /* perform subtraction */
	      tmp = lt;
	      lt = bigint_sub(lt,rt);
	      bigint_free(tmp);
	      /* change lsb from zero to one */
	      q->blks[0] |= 1;
	    }
	  tmp = rt;
	  rt = bigint_shift_right(rt,1);
	  bigint_free(tmp);
	  shift--;
	}
      /* correct the sign of the result */
      if(negative)
	{
	  tmp = bigint_negate(q);
	  bigint_free(q);
	  q = tmp;
	}
    }
  bigint_free(rt);
  bigint_free(lt);
  return q;
}

/********************************************************/
/* Test and compare functions                           */
/********************************************************/
inline int bigint_le(bigint l, bigint r)
{
  return (bigint_cmp(l, r) < 1);
}

/********************************************************/
inline int bigint_lt(bigint l, bigint r)
{
  return (bigint_cmp(l, r) == -1);
}

/********************************************************/
inline int bigint_ge(bigint l, bigint r)
{
  return (bigint_cmp(l, r) > -1);
}

/********************************************************/
inline int bigint_gt(bigint l, bigint r)
{
  return (bigint_cmp(l, r) == 1);
}

/********************************************************/
inline int bigint_eq(bigint l, bigint r)
{
  return (!bigint_cmp(l, r));
}

/********************************************************/
inline int bigint_ne(bigint l, bigint r)
{
  return abs(bigint_cmp(l, r));
}

/********************************************************/
/* binary read and write */
/********************************************************/
void bigint_write_binary(FILE *f,bigint x)
{
  if(fwrite(&(x->size),sizeof(x->size),1,f)!=1)
    {
      perror("Write failed");
      exit(4);
    }
  if(fwrite(x->blks,sizeof(chunk),x->size,f)!=x->size)
    {
      perror("Write failed");
      exit(4);
    }
}

/********************************************************/
bigint bigint_read_binary(FILE *f)
{
  bigint r;
  if((r = (bigint)malloc(sizeof(struct bigint_struct))) == NULL)
    {
      perror("bigint_read_binary");
      exit(1);
    }
  if(fread(&(r->size),sizeof(r->size),1,f)!=1)
    {
      free(r);
      return NULL;
    }
  r->blks = (chunk*)malloc(r->size * sizeof(chunk));
  if(r->blks == NULL)
    {
      perror("bigint_read_binary");
      exit(2);
    }
  if(fread(r->blks,sizeof(chunk),r->size,f)!=r->size)
    {
      perror("Unable to read from file");
      exit(4);
    }
  return r;  
}
