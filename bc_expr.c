/*******************************************************************************
* bc_expr.c
*******************************************************************************/

#include       <stdio.h>
#include       <ctype.h>
#include       <stdlib.h>
#include       "bc.h"
#include       "errors.h"
#include       "tokensv2.h"
#include       "tokensbl.h"

/*******************************************************************************
* #defines,typedefs
*******************************************************************************/

#define        TSMAX                  31

/*******************************************************************************
* Globals
*******************************************************************************/

int            Typestack[TSMAX+1];
int            TypeSP;

/*******************************************************************************
* Forward
*******************************************************************************/

int            Expression2(void);

/*******************************************************************************
* void PushTypestack(int type)
*******************************************************************************/

void PushTypestack(int type)
{
  if (TypeSP > TSMAX)
    Error(E_FORMTOOCPLX);
  Typestack[TypeSP] = type;
  TypeSP++;
}

/*******************************************************************************
* int PopTypestack(voiod)
*******************************************************************************/

int PopTypestack(voiod)
{
  TypeSP--;
  return Typestack[TypeSP];
}

/*******************************************************************************
* U8 MakeRelop(U8 c)
* c is either '<', '>' or '=' (tokenized) . Check next char and return
* a BL_ operator
*******************************************************************************/

U8 MakeRelop(U8 c)
{
  U8           c2;
  
  if (c == V2_EQL)
    return BL_EQL;
    
  c2 = GetChr();
  
  if (c == V2_GTR)
  {
    if (c2 == V2_EQL)
      return BL_GEQ;
    UngetChr();  
    return BL_GTR;
  }
  // V2_LSS
  if (c2 == V2_EQL) return BL_LEQ;
  if (c2 == V2_GTR) return BL_NEQ;
  
  UngetChr();
  return BL_LSS;
}

/*******************************************************************************
* int GenIndices(void)
* Generate code for all array indice(s), return nr. indices
*******************************************************************************/

int GenIndices(void)
{
  int          n,type;
  U8           c;
  
  n = 0;
  do 
  {
    type = Expression2();                    // generate code to push the dimension(s)
    if (type != T_NUM)
      Error(E_TYPEMISMATCH);
    n++;
    c = GetChr();
  } while (c == ',');  
  UngetChr();
  return n;
}

/*******************************************************************************
* void DoNumNumfunc(U8 pCode)
* most BASIC functions take a num. parameter and return a num.
*******************************************************************************/

void DoNumNumfunc(U8 pCode)
{
  int          type;
  
  SynChk('(');
  type = Expression2();
  if (type != T_NUM)
    Error(E_TYPEMISMATCH);
  SynChk(')');
  GenByte(pCode);
  PushTypestack(T_NUM);
}        

/*******************************************************************************
* void DoStrNumfunc(U8 pCode)
* BASIC funcs that take a STRING param. and return a num. (LEN,VAL,ASC)
*******************************************************************************/

void DoStrNumfunc(U8 pCode)
{
  int          type;
  
  SynChk('(');
  type = Expression2();
  if (type != T_STR)
    Error(E_TYPEMISMATCH);
  SynChk(')');
  GenByte(pCode);
  PushTypestack(T_NUM);
}

/*******************************************************************************
* void DoNumStrfunc(U8 pCode)
* BASIC functions that take a num. parameter and return a string (STR$,CHR$)
*******************************************************************************/

void DoNumStrfunc(U8 pCode)
{
  int          type;
  
  SynChk('(');
  type = Expression2();
  if (type != T_NUM)
    Error(E_TYPEMISMATCH);
  SynChk(')');
  GenByte(pCode);
  PushTypestack(T_STR);
}        

/*******************************************************************************
* void DoStrfunc(U8 pCode)
*******************************************************************************/

void DoStrfunc(U8 pCode)
{            
  int          type;
  
  SynChk('(');
  type = Expression2();
  if (type != T_STR)
    Error(E_TYPEMISMATCH);
  SynChk(',');
  type = Expression2();
  if (type != T_NUM)
    Error(E_TYPEMISMATCH);
  if (pCode == BL_MIDs)
  {
    SynChk(',');
    type = Expression2();
    if (type != T_NUM)
      Error(E_TYPEMISMATCH);
  }
  SynChk(')');
  GenByte(pCode);
  PushTypestack(T_STR);
}

/*******************************************************************************
* void Primary(void)
* Evaluate an operand
*******************************************************************************/

void Primary(void)
{
  U8           c;
  VARID        ID;
  U16          name;
  BOOL         bIndexed;
  int          type;
  int          i,n;
  int          l,s;
  int          numres;
  BOOL         bWarn,bDone;
  int          intnum;
  U8           CBMFloat[5];
  U8           tmpstring[80];
  int          patchloc;
  
  // try var
  
  name = ReadVarname(&type,&bIndexed);
  if (name != 0)                       
  {                                    // it's a var
    if (bIndexed)                      // an array ?
    {
      n  = GenIndices();
      SynChk(')');
      ID = RefArray(name,n,&bWarn);
    }  
    else
    {
      if (name == 0x4954)              // TI ?
      {
        GenByte(BL_TI); PushTypestack(T_NUM); return; 
      }
      if (name == 0x5453)              // ST ?
      {
        GenByte(BL_ST); PushTypestack(T_NUM); return; 
      }
      if (name == 0xC954)              // TI$ ?
      {
        GenByte(BL_TISTR); PushTypestack(T_STR); return; 
      }
      ID = RefVar(name);
    }
    GenPushVar(ID,type,bIndexed);
    if ((type == T_FLT) || (type == T_INT) )
      PushTypestack(T_NUM);
    else  
      PushTypestack(T_STR);
    return;                            // Done
  }
  
  // try num. constant
  
  numres = ReadNum(&intnum,CBMFloat);
  if (numres != N_NONUM)
  {                                    // it's a num. constant
    if (numres == N_INT)
      GenPushIntconst(intnum);
    else  
      GenPushFloatconst(CBMFloat);
      
    PushTypestack(T_NUM);
    return;                            // Done
  }
  
  // Get next char and test various options
  
  c = GetChr();
  
  // try PI
  
  if (c == V2_PI)
  {
    GenByte(BL_PI);
    PushTypestack(T_NUM);
    return;
  }  
  
  // try string literal
  
  if (c == 34)                         // double quote ?
  {
    n = -1;                            // stringlength
    bDone = FALSE;
    do
    {
      n++;
      c = GetChrAll();                    
      if ( (c == 34) || (c == 0) )        // don't store quote
        bDone = TRUE;
      else  
        tmpstring[2+n] = c;            // 0,1 : leave room for length byte(s)
    } while (!bDone);
    if (c != 34)
      UngetChr();
    if (n < 8)
    {
      tmpstring[1] = BL_PUSHSTR0+n; l=n+1; s=1; 
    }  
    else
    { 
      tmpstring[0] = BL_PUSHSTRN;
      tmpstring[1] = n;      l=n+2; s=0;
    }
    for (i=0; i<l; i++)
      GenByte(tmpstring[s++]);
    PushTypestack(T_STR);
    return;
  }
  
  // try user defined function
  
  if (c == V2_FN)
  {
    name = ReadVarname(&type,&bIndexed);
    if (type != T_FLT)
      Error(E_FLTEXP);
    if (!bIndexed)     // indexed just means a '(' is behind the varname
      Error(E_SYNTAX);
    name = VarMakefunc(name);  
    ID = RefFunc(name);  
    if ( (Pass == 2) && (ID == -1) )
      Error(E_FNNOTDEF);               // undefined function = error
    type = Expression2();  
    SynChk(')');
    GenByte(BL_CALLFN);
    GenWord((U16)ID,HIFIRST);
    PushTypestack(T_NUM);
    return;
  }
  
// try BASIC function
  
  if ( (c >= V2_SGN) && (c <= V2_MIDs) )
  {
    switch (c)
    {
      case V2_LEN    :
      case V2_VAL    :
      case V2_ASC    : DoStrNumfunc((U8)(c-0x94));              // 0x94 : BASIC function tokens start at 0xb4,
                       break;                                   // corresponding BL_ functions at 0x20
      case V2_STRs   :              
      case V2_CHRs   : DoNumStrfunc((U8)(c-0x94));
                       break;
      case V2_LEFTs  : DoStrfunc(BL_LEFTs);
                       break;
      case V2_RIGHTs : DoStrfunc(BL_RIGHTs);               
                       break;
      case V2_MIDs   : DoStrfunc(BL_MIDs);                 
                       break;
      default        : DoNumNumfunc((U8)(c-0x94)); 
                       break;
    }
    return;
  }
  
  // must be left parenthesis, else error
  
  if (c == '(')
  {
    PushTypestack(Expression2());
    SynChk(')');
    return;
  }  
  Error(E_SYNTAX);
}


/*******************************************************************************
* void ExponentExpression(void)
* ! exponentation is right-associative, but NOT in BASIC V2 !
*******************************************************************************/

void ExponentExpression(void)
{
  U8           c;
  int          t1,t2;
  
  Primary();
  c = GetChr();
  while (c == V2_EXPN)
  {
    Primary();
    t2 = PopTypestack(); t1 = PopTypestack();
    if ( (t1 != T_NUM) || (t2 != T_NUM) )
      Error(E_TYPEMISMATCH);
    GenByte(BL_EXPN);
    PushTypestack(T_NUM);
    c = GetChr();
  }
  UngetChr();
}

/*******************************************************************************
* void UnaryExpression(void)
*******************************************************************************/

void UnaryExpression(void)
{
  U8           c;
  int          t1;
  
  c = GetChr();
  if ( (c == V2_SUB) || (c == V2_ADD) )
  {
    UnaryExpression();                   // unary + or -  associate right-to-left, call self first
    t1 = PopTypestack();
    if (t1 != T_NUM)
      Error(E_TYPEMISMATCH);
    if (c == V2_SUB)                     // ignore unary '+'
      GenByte(BL_UMINUS);
    PushTypestack(T_NUM);
  }
  else
  {
    UngetChr();
    ExponentExpression();
  }
}

/*******************************************************************************
* void MultExpression(void)
*******************************************************************************/

void MultExpression(void)
{
  U8           c;
  int          t1,t2;
  
  UnaryExpression();
  c = GetChr();
  while ( (c == V2_MUL) || (c == V2_DIV) )
  {
    UnaryExpression();
    t2 = PopTypestack(); t1 = PopTypestack();
    if ( (t1 != T_NUM) || (t2 != T_NUM) )
      Error(E_TYPEMISMATCH);
    if (c == V2_MUL)  
      GenByte(BL_MUL);
    else  
      GenByte(BL_DIV);
    PushTypestack(T_NUM);
    c = GetChr();
  }
  UngetChr();
}

/*******************************************************************************
* void AddExpression(void)
* also handles string concatenation
*******************************************************************************/

void AddExpression(void)
{
  U8           c;
  int          t1,t2;
  
  MultExpression();
  c = GetChr();
  while ( (c == V2_ADD) || (c == V2_SUB) )
  {
    MultExpression();
    t2 = PopTypestack(); t1 = PopTypestack();
    if (t2 != t1)
      Error(E_TYPEMISMATCH);
    if ( (t1 == T_STR) && (c == V2_SUB) )  
      Error(E_SYNTAX);                 // cannot subtract strings
    if (c == V2_ADD)
      GenByte(BL_ADD);
    else  
      GenByte(BL_SUB);
    PushTypestack(t1);                 // either T_NUM or T_STR
    c = GetChr();
  }
  UngetChr();
}

/*******************************************************************************
* void RelationalExpression(void)
* 
* BASIC V2 only crunches <,>,=, not <= etc. So some extra work.
* Also handle string comparisons (Blitz runtime uses same operators 
* for strings and numbers)
*******************************************************************************/

void RelationalExpression(void)
{
  U8           c;
  int          t1,t2;
  U8           opr;
  
  AddExpression();
  c = GetChr();
  while ( (c == V2_LSS) || (c == V2_GTR) || (c == V2_EQL) )
  {
    opr = MakeRelop(c);                // returns BL_ operator
    AddExpression();
    t2 = PopTypestack(); t1 = PopTypestack();
    if (t2 != t1)
      Error(E_TYPEMISMATCH);
    GenByte(opr);  
    PushTypestack(T_NUM);
    c = GetChr();  
  }
  UngetChr();
}

/*******************************************************************************
* void NotExpression(void)
* NOT associates right-to-left
*******************************************************************************/

void NotExpression(void)
{
  U8           c;
  int          t1;
  
  c = GetChr();
  if (c == V2_NOT)
  {
    NotExpression();                   // call self first, RL-assc.
    t1 = PopTypestack();
    if (t1 != T_NUM)
      Error(E_TYPEMISMATCH);
    GenByte(BL_NOT);  
    PushTypestack(T_NUM);
  }  
  else
  {
    UngetChr();
    RelationalExpression();
  }
}

/*******************************************************************************
* void AndExpression(void)
*******************************************************************************/

void AndExpression(void)
{
  U8           c;
  int          t1,t2;
  
  NotExpression();
  c = GetChr();
  while (c == V2_AND)
  {
    NotExpression();
    t2 = PopTypestack(); t1 = PopTypestack();
    if ( (t1 != T_NUM) || (t2 != T_NUM) )
      Error(E_TYPEMISMATCH);
    GenByte(BL_AND);
    PushTypestack(T_NUM);
    c = GetChr();
  }
  UngetChr();
}

/*******************************************************************************
* void OrExpression(void)
*******************************************************************************/

void OrExpression(void)
{
  U8           c;
  int          t1,t2;
  
  AndExpression();
  c = GetChr();
  while (c == V2_OR)
  {
    AndExpression();
    t2 = PopTypestack(); t1 = PopTypestack();
    if ( (t1 != T_NUM) || (t2 != T_NUM) )
      Error(E_TYPEMISMATCH);
    GenByte(BL_OR);
    PushTypestack(T_NUM);
    c = GetChr();
  }
  UngetChr();
}

/*******************************************************************************
* int Expression2(void)
*******************************************************************************/

int Expression2(void)
{
  OrExpression();
  return (PopTypestack());
}

/*******************************************************************************
* int Expression(void)
* return T_NUM or T_STR
*******************************************************************************/

int Expression(void)
{
  TypeSP = 0;
  return (Expression2());
}

/*******************************************************************************
* void NumExpression(void)
*******************************************************************************/

void NumExpression(void)
{
  if (Expression() != T_NUM)
    Error(E_TYPEMISMATCH);
}

/*******************************************************************************
* void StringExpression(void)
*******************************************************************************/

void StringExpression(void)
{
  if (Expression() != T_STR)
    Error(E_TYPEMISMATCH);
}

