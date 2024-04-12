/*******************************************************************************
* bc_readnum.c
*******************************************************************************/

#include       <stdio.h>
#include       <ctype.h>
#include       <stdlib.h>
#include       "bc.h"
#include       "errors.h"

/*******************************************************************************
* Union to access double-bytes
*******************************************************************************/

typedef union
{
  double       IEEE754Double;          // 8 bytes
  U8           Dbytes[8];
} FLTU;


/*******************************************************************************
* Globals, 
*******************************************************************************/

static U8      buff[80];
static int     n;
static U8      c;

/*******************************************************************************
* void MakeCBMFloat(double d,U8 *pCBM)
* 
* conversion for IEEE double to CBM float.
*
* IEEE754 double : sign, 11 bit exponent, bias = 1023, 52 bits significand (53).
* CBMFloat       : 8 bit exponent, bias = 128, sign, 31 bits significand (32).
*
* Both formats have an implied '1' bit at the start of the significand.
* IEEE has LSB first, CBM MSB first.
*
* Does not handle IEEE denormals etc.
*******************************************************************************/

void MakeCBMFloat(double d,U8 *pCBM)
{
  FLTU         CV;
  U8           Sign;
  int          Exponent;
  int          i;
  
  CV.IEEE754Double = d;
  Sign = CV.Dbytes[7] & 0x80;                 // sign = MSBit
  Exponent =  (CV.Dbytes[7] & 0x7f) << 4;     // 7 digits exponent ..
  Exponent |= ((CV.Dbytes[6] & 0xf0) >> 4);   // .. plus 4 more
  if (Exponent != 0)                          // special case: exponent is zero, don't change it
  {
    Exponent -= 1022;                         // CBM Needs exponent offset +1, uses exp==0 for 0.000
    if ( (Exponent < -128) || (Exponent > 127) )
      Error(E_OVERFLOW);
    pCBM[0] = Exponent + 128;  
  } 
  else
  {
    pCBM[0] = 0;
  }

  // signif. starts at bit 3 from Dbytes[6], CBM needs it in bit 6  
  // shift signicand 3 places to the left
  
  CV.Dbytes[6] &= 0x0f;                // get rid of exponent bits
  for (i=0; i<3; i++)
  {
    CV.Dbytes[6] <<= 1; if (CV.Dbytes[5] & 0x80) CV.Dbytes[6] |= 0x01;
    CV.Dbytes[5] <<= 1; if (CV.Dbytes[4] & 0x80) CV.Dbytes[5] |= 0x01;
    CV.Dbytes[4] <<= 1; if (CV.Dbytes[3] & 0x80) CV.Dbytes[4] |= 0x01;
    CV.Dbytes[3] <<= 1; if (CV.Dbytes[2] & 0x80) CV.Dbytes[3] |= 0x01;
    CV.Dbytes[2] <<= 1; 
  }  
  pCBM[1] = CV.Dbytes[6] | Sign; 
  pCBM[2] = CV.Dbytes[5]; 
  pCBM[3] = CV.Dbytes[4]; 
  pCBM[4] = CV.Dbytes[3]+( (CV.Dbytes[2] & 0x80) ? 1 : 0 ); 
}

/*******************************************************************************
* void StoBuff(void)
* Store 'c' into buff.
*******************************************************************************/

void StoBuff(void)
{
  buff[n] = c; n++;
}

/*******************************************************************************
* void GetDigs(void)
* Get digits and store in buff/ebuff
*******************************************************************************/

void GetDigs(void)
{
  while (isdigit(c))
  {
    StoBuff();
    c = GetChr();
  }
}

/*******************************************************************************
* int ReadNum(int *pInt,U8 *pFlt)
*
* First move all possible characters to 'buff', then convert from there
* There's no leading - or +, already processed as unary operator
* Overflow checking is done in the MakeCBMFloat function
*
* Return : N_NONUM,N_INT or N_FLT, integer or CBMFloat
*******************************************************************************/

int ReadNum(int *pInt,U8 *pFlt)  // Add BOOL unsigned integers, Expression(BOOL bUnsigned) for peek/poke/sys/usr
{
  double       d,e;
  int          iNum;
  int          rv;
  BOOL         bGotsign;               // exponent : already have sign
  
  for (n=0; n<80; n++)
    buff[n]  = 0x00;
  n = 0;
  bGotsign = FALSE;
  c = GetChr();
  
  // start with digit(s) or decimal point
  
  if ( (!isdigit(c)) && (c != '.') ) 
  {
    UngetChr();
    return N_NONUM;
  }  
  GetDigs();  
  if (c == '.')
  {
    StoBuff();
    c = GetChr();
  }  
  
  // now more digits or 'E'
  
  GetDigs();
  if (c == 'E')
  {
    StoBuff();
    c = GetChr();
    if ( (c == '-') || (c == '+') )
    {
      if (bGotsign)
        Error(E_NUMFMT);
      bGotsign = TRUE;
      StoBuff();
      c = GetChr();
    }
    GetDigs();                         // !! check no more than 2 digits in the exponent !!
  }
  
  // Done scanning. Now eval the number
  
  d = atof(buff);
  iNum = (int)d;                       // to int
  e = (double)iNum;                    // and back to float
  
  // can 'd' be represented as an integer ?
  
  if ( (iNum > 32767) || (e != d) )
  {
    MakeCBMFloat(d,pFlt);         // no, convert to 5 bytes CBM
    rv = N_FLT;
  }
  else
  {        
    *pInt = iNum;                  // yes
    rv = N_INT;
  }
  UngetChr();
  return rv;
}

