/*******************************************************************************
* bc_genpc.c
* Generate Pcode
*******************************************************************************/

#include       <stdio.h>
#include       <ctype.h>
#include       "bc.h" 
#include       "errors.h"
#include       "tokensbl.h"

/*******************************************************************************
* Vars
*******************************************************************************/

U8             Code[49152];            // buffer for pcode
int            iCode;                  // index into Code
BOOL           bBufmode;
U8             *pWrite;
int            iBuff;

/*******************************************************************************
* void InitGen(void)
*******************************************************************************/

void InitGen(void)
{
  iCode   = 0;
  bBufmode = FALSE;
}  

/*******************************************************************************
* void GenByte(U8 c)
* add byte (c) to top of Code[] , bump ptr
* In Pass 1, no data is generated, just bump the code index;
*******************************************************************************/

void GenByte(U8 c)
{
  int          i;
  
  if (bBufmode)
  {
    pWrite[iBuff] = c; iBuff++; pWrite[0]++;
  }
  else
  {
    Code[iCode] = c;
    iCode++;
  }  
}

/*******************************************************************************
* void GenWord(U16 w,int Order)  Order = LOFIRST or HIFIRST
* In Pass 1, no data is generated, just bump the code index;
*******************************************************************************/

void GenWord(U16 w,int Order)
{
  int          i;
  
  if (Order == LOFIRST)
  {
    GenByte((U8)(w & 0x00ff));
    GenByte((U8)((w >> 8) & 0x00ff));
  }
  else
  {
    GenByte((U8)((w >> 8) & 0x00ff));
    GenByte((U8)(w & 0x00ff));
  }
}

/*******************************************************************************
* int GenGetIndex(void)
* return Code index (used for patchups)
*******************************************************************************/

int GenGetIndex(void)
{
  return iCode;
}

/*******************************************************************************
* void GenSetbufmode(U8 *pBuff)
*******************************************************************************/

void GenSetbufmode(U8 *pBuff)
{
  if (pBuff != NULL)
  {
    pWrite = pBuff;
    bBufmode = TRUE;
    pWrite[0] = 0;                     // length byte
    iBuff = 1;                         // starting write pos.
  }  
  else  
  {
    bBufmode = FALSE;
  }
}

/*******************************************************************************
* void GenPatch(int Index,U8 newbyte)
*******************************************************************************/

void GenPatch(int Index,U8 newbyte)
{  
  if (Pass == 2)
    Code[Index] = newbyte;
}

/*******************************************************************************
* void Listcode(int start,int end
*******************************************************************************/

void Listcode(int start,int end)
{
  int          i,cnt;
  
//  List("%5d: ($%04X) ",curLine,start+ProgStart);
  List("%5d: (%5d) ($%04X) ",curLine,start+ProgStart,start+ProgStart);
  cnt = 0;
  for (i=start; i<end; i++)
  {
    List("%02X ",Code[i]);
    cnt++;
    if (cnt == 8)
    {
      ListCR();
      List("                       ");
      cnt = 0;
    }
  }
  ListCR();
}

/*******************************************************************************
* void GenIntconstBuff(int c,U8 *buff,int *n)
* The code to generate pushing an int. constant is used in several places,
* generate it here in a buffer and set the buflen 'n'
*******************************************************************************/

void GenIntconstBuff(int c,U8 *buff,int *n)
{
  int          b;
  
  if (c < 16)
  {
    buff[0] = ((U8)(BL_PUSHCS1 | (c & 0x0f)));     // B0..BF = push 0..15
    *n = 1;
    return;
  }
  if (c < 32)
  {
    buff[0] = ((U8)(BL_PUSHCS2 | ((c-16) & 0x0f)));     // F0..FF = push 16..31
    *n = 1;
    return;
  }
  if (c < 256)
  {
    buff[0] = BL_PUSHC;
    buff[1] = c;
    *n = 2;
    return;
  }
  buff[0] = BL_PUSHC2;
  buff[1] = (U8)((c>>8) & 0xff);                 // HI first !
  buff[2] = (U8)(c & 0xff);
  *n = 3;
}

/*******************************************************************************
* void GenPushIntconst(int c)
* consts are always entered >0, negated afterwards if negative
*******************************************************************************/

void GenPushIntconst(int c)
{
  U8           buff[3];
  int          i,n;
  
  GenIntconstBuff(c,buff,&n);
  for (i=0; i<n; i++)
    GenByte(buff[i]);
}

/*******************************************************************************
* void GenPushFloatconst(U8 *pFloat)
*******************************************************************************/

void GenPushFloatconst(U8 *pFloat)
{
  int          i;
  
  GenByte(BL_PUSHFLT);
  for (i=0; i<5; i++)
    GenByte(*(pFloat++));
}

/*******************************************************************************
* void GenStoVar(VARID ID)
* store TOS into var(ID)
*******************************************************************************/

void GenStoVar(VARID ID)
{
  if (ID < 32) 
  {
    GenByte((U8)(BL_STOVARQ+ID));                   // $C0..$DF
  }  
  else  
  {
    if (ID < 256)
    {
      GenByte(BL_STOVAR);                        // $E0 xx
      GenByte((U8)ID);
    }  
    else
    {
      if (ID < 512)
      {
        GenByte((U8)(BL_STOVAR2+(ID-256)));         // E1 xx  !! different from E0
      }
    }
  }  
}

/*******************************************************************************
* void GenStoArray(VARID ID)
* store TOS into array(ID)
*******************************************************************************/

void GenStoArray(VARID ID)
{
  if (ID < 256) 
  {
    GenByte(BL_STOARRAY); GenByte((U8)ID);
  }  
  else  
  {
    GenByte(BL_STOARRAY2); GenWord((U16)(ID),LOFIRST);
  }  
}

/*******************************************************************************
* void GenPushVar(VARID ID,int type,BOOL bIndexed)
* for any scalar     : 80..9f, then A0 20,  A0 21
* for arrays         : A4 0x  ,  A5 ll hh
*******************************************************************************/

void GenPushVar(VARID ID,int type,BOOL bIndexed)
{
  if (bIndexed)                                  // an array ?
  {
    if (ID < 256)                             // no diffence for string/numeric vars
    {
      GenByte(BL_PUSHARR); GenByte((U8)(ID));
    }  
    else
    {
      GenByte(BL_PUSHARR2); GenWord((U16)(ID),LOFIRST);
    }
    return;
  }
  
  // scalars
  
  if (ID < 32)                                // first 32 stringvars use 80..9f
  {
    GenByte((U8)(BL_PUSHV+ID));
    return;
  }
  if (ID < 256)
  {
    GenByte(BL_PUSHA); GenByte((U8)ID);
  }  
  else  
  {
    GenByte(BL_PUSHA2); GenWord((U16)ID,LOFIRST);
  }

}

/*******************************************************************************
* void GenPushVarAddress(VARID ID,int type,BOOL bIndexed)
* for numeric scalar : A0 0x, A1 xx xx
* for string  scalar : 80..9f, then A0 20,  A0 21
* for arrays         : A4 0x  ,  A5 ll hh
*******************************************************************************/

void GenPushVarAddress(VARID ID,int type,BOOL bIndexed)
{
  // arrays
  
  if (bIndexed)                                  // an array ?
  {
    if (ID < 256)                               // no diffence for string/numeric vars
    {
      GenByte(BL_PUSHARR); GenByte((U8)ID);
    }  
    else
    {
      GenByte(BL_PUSHARR2); GenWord((U16)ID,LOFIRST);
    }
    return;
  }
  
  // scalars
  
  if ( (type == T_STR) && (ID < 32) )            // first 32 stringvars use 80..9f
  {
    GenByte((U8)(BL_PUSHV+ID));
    return;
  }
  if (ID < 256)
  {
    GenByte(BL_PUSHA); GenByte((U8)ID);
  }  
  else  
  {
    GenByte(BL_PUSHA2); GenWord((U16)ID,LOFIRST);
  }
}

/*******************************************************************************
* void CopyCode(int *pIx)
*******************************************************************************/

void CopyCode(int *pIx)
{
  int          i;
  
  for (i=0; i<iCode; i++)
  {
    Pdata[*pIx] = Code[i];
    (*pIx)++;
  }  
}

