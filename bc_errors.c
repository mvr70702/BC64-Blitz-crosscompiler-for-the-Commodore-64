/*******************************************************************************
* bc_errors.c
* Errors,warnings
*******************************************************************************/

#include       <stdio.h>
#include       <setjmp.h>
#include       <stdlib.h>
#include       "bc.h"
#include       "errors.h"

extern         jmp_buf        JB;      // comp.c

/*******************************************************************************
* Defs, vars
*******************************************************************************/

typedef struct
{
  int          ecode;
  char         *pMsg;
} ERR;


int            nErrors;
int            nWarnings;

/*******************************************************************************
* void InitErr(void)
*******************************************************************************/

void InitErr(void)
{
  nErrors = 0;
  nWarnings = 0;
}

/*******************************************************************************
* int GetNrErrors(void)
*******************************************************************************/

int GetNrErrors(void)
{
  return nErrors;
}

/*******************************************************************************
* int GetNrWarnings(void)
*******************************************************************************/

int GetNrWarnings(void)
{
  return nWarnings;
}

/*******************************************************************************
* Errormessages
*******************************************************************************/

ERR            Errors[] = {

  { E_SYNTAX       , "Syntax"                },
  { E_LINENUM      , "Linenum too big"       },
  { E_FLTEXP       , "Float name expected"   },
  { E_EXPECTED     , "Expected"              },
  { E_NUMFMT       , "Number format"         },
  { E_FNNOTDEF     , "Undefined function"    },
  { E_VAREXP       , "Var expected"          },
  { E_TYPEMISMATCH , "TYpe mismatch"         },
  { E_OVERFLOW     , "Overflow"              },
  { E_FORMTOOCPLX  , "Formula too complex"   },
  { E_ILLASSIGN    , "Illegal assignment"    },
  { E_ILLCMD       , "Illegal command"       },
  { E_UNDEFSTT     , "Undef'd statement"     },
  { E_FIOVF        , "FI table overflow"     },
  { E_UNKNOWN      , "Unknown BASIC token"   },
  { E_MEMFULL      , "Program too big"       },
  
  { -1             , NULL                    },
};

ERR            Warnings[] = {

  { W_DIMCHANGED   , "array dimensions changed " },
  { W_REDIM        , "array redimensioned      " },
  { -1             , NULL                        },
};  

/*******************************************************************************
* void SynChk(U8 c)
* GetChr(), Error if not == c
*******************************************************************************/

void SynChk(U8 c)
{
  if (GetChr() != c)
  {
    if (c >= 0x80)
      printf("%02x ",c);
    else  
      printf("%c  ",c);
    Error(E_EXPECTED);
  }  
}

/*******************************************************************************
* void Error(int e)
* output message to STDOUT and listfile
*******************************************************************************/

void Error(int e)
{
  ERR          *pErr;
  
  pErr = &Errors[0];
  
  while (pErr->ecode >= 0)
  {
    if (pErr->ecode == e)
    {
      printf("%s error in line %5d pos %2d *******\n\r",pErr->pMsg,curLine,GetBufpos());
      List  ("%s error in line %5d pos %2d *******\n\r",pErr->pMsg,curLine,GetBufpos());
    }
    pErr++;
  }
  if (Pass == 2)
    nErrors++;
  longjmp(JB,2);                       // Exit to bc_comp.c (skips rest of line, continue with next line)
}

/*******************************************************************************
* void Warn(int w)
*******************************************************************************/

void Warn(int w)
{
  ERR          *pWarn;
  
  pWarn = &Warnings[0];
  
  while (pWarn->ecode >= 0)
  {
    if (pWarn->ecode == w)
    {
      printf("Warning : %s in line %5d pos %2d *******\n\r",pWarn->pMsg,curLine,GetBufpos());
      List  ("Warning : %s in line %5d pos %2d *******\n\r",pWarn->pMsg,curLine,GetBufpos());
    }
    pWarn++;
  }
  if (Pass == 2)
    nWarnings++;
}


