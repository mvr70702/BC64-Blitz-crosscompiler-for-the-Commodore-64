/*******************************************************************************
* bc_comptok.c
* compile token functions
* functions are ordered by their tokens (tokensV2.h)
*******************************************************************************/

#include       <stdio.h>
#include       <ctype.h>
#include       "bc.h"
#include       "errors.h"
#include       "tokensV2.h"
#include       "tokensbl.h"


/*******************************************************************************
* OPEN/LOAD/SAVE/VERIFY/POKE/WAIT all must generate code from end of arguments
* to start. So first gen. to one of these buffers, later produce pcode in
* the right order
*******************************************************************************/

static U8      CBuff1[128];
static U8      CBuff2[128];
static U8      CBuff3[128];
static U8      CBuff4[128];

static U8      *CodeBuffs[] = {
  CBuff1,
  CBuff2,
  CBuff3,
  CBuff4,
};

/*******************************************************************************
* int PushArrayIndices(void)
* Generate code for all indices, return nr. indices
*******************************************************************************/

int PushArrayIndices(void)
{
  int          n;
  U8           c;
  
  n = 0;
  do 
  {
    NumExpression();                         // generate code to push the dimension(s)
    n++;
    c = GetChr();
  } while (c == ',');  
  UngetChr();
  return n;
}

/*******************************************************************************
* int ReadLinenum(BOOL bMust)
* if 'bMust', a valid linenr. must be read (GOTO...), else error
* if not bMust, just try, but if there's a number, if must be valid
*******************************************************************************/

int ReadLinenum(BOOL bMust)
{
  int          t,linenum;
  U8           CBMFloat[5];
  
  t = ReadNum(&linenum,CBMFloat);            // read the linenumber     
  if (t != N_INT)                            // must be an int
  { 
    if (bMust)
      Error(E_SYNTAX);
    else  
      return -1;
  }    
  else
  {
    if (linenum > 63999)
      Error(E_LINENUM);
  }
  return linenum;
}

/*******************************************************************************
* void Input(void)
* code for INPUT/INPUT#
* intro ($50 for INPUT, channel $48 for INPUT#) is already generated.
*******************************************************************************/

void Input(void)
{
  U16          name;
  int          type;
  BOOL         bIndexed,bDone,bWarn;
  int          cnt,n;
  VARID        ID;
  
  cnt = 0; bDone = FALSE;
  do
  {
    name = ReadVarname(&type,&bIndexed);               
    if (name == 0)
      Error(E_VAREXP);
    if (bIndexed)                            // array : must push index(es) first
    {
      n = PushArrayIndices();                // generates code
      ID = RefArray(name,n,&bWarn);
      if (bWarn)
        Warn(W_DIMCHANGED);
      SynChk(')');
    }
    else
    {
      ID = RefVar(name);                      // enter or get
    }
    GenPushVarAddress(ID,type,bIndexed);      // push an address
    if (TryChr(','))
    {  
      if (cnt == 0)
        GenByte(0x55);
      else  
        GenByte(0x57);
    }
    else
    {
      if (cnt == 0)
        GenByte(0x53);
      else  
        GenByte(0x51);
      bDone = TRUE;  
    }
    cnt++;
  } while (!bDone);
}

/*******************************************************************************
* void Print(U8 Token)
* code for PRINT/PRINT#
*
* This looks complex. but so is the BASIC PRINT statement. 
* It can contain expressions,commas,semicolons,TAB(),SPC, without separators
* 
* Here's the Pcodes used for PRINT
* 
* $3B          BL_TAB10                Print comma
* $3C          BL_PRTOS                Print TOS (no CR)
* $3D          BL_PRTOSTAB10           Print TOS , (comma)
* $3E          BL_PRTOSCR              Print TOS, followed by a CR
* $3F          BL_PRCR                 Print CR
* $40          BL_SPC                  Print (TOS) spaces
* $41          BL_TAB                  Tab to (TOS)
*
* PRINT# used mostly the same codes, with these exceptions
*
* $42          BL_CMD                  Start of any PRINT# statement
* $43          BL_PRNTOSCR             Same as $3E for PRINT
* $44          BL_PRNCR                Same as $3F for PRINT
* $45          BL_PRNTOS               Added (!) after $3C
*
*
* pstate <> 0  :  something to print on stack (FAC) : expression was called
* bSuppress    :  comma or semicolon caused last print
*******************************************************************************/

void Print(U8 Token)
{
  int          pstate;
  BOOL         bSuppress;
  U8           c;
  
  pstate = 0; bSuppress = FALSE;          // nothing on stack, no ';'
  do                
  {
    c = GetChr();
    switch (c)
    {
      case 0x00   : 
      case ':'    : UngetChr();           // compileline must find 00 or ':', so unget
                    if (pstate) 
                    {
                      if (bSuppress)
                      {
                        GenByte(BL_PRTOS); // PRTOS_NOCR
                        if (Token == V2_PRINTN)
                          GenByte(BL_PRNTOS);
                      }  
                      else                                
                      {
                        if (Token == V2_PRINTN)
                          GenByte(BL_PRNTOSCR);
                        else  
                          GenByte(BL_PRTOSCR);  //PRTOS_CR
                      }  
                    }    
                    else  
                    {
                      if (!bSuppress)
                      {
                        if (Token == V2_PRINTN)
                          GenByte(BL_PRNCR);
                        else  
                          GenByte(BL_PRCR);    // PR_CR
                      }    
                      else
                      {
                        if (Token == V2_PRINTN)
                          GenByte(BL_PRNTOS);
                      }
                    }  
                    break;
      case ','    : if (pstate)              
                    {
                      GenByte(BL_PRTOSTAB10);      // PRTOS_COMMA
                      pstate = 0;
                    }  
                    else  
                    {
                      GenByte(BL_PRTAB10);        // PR_COMMA
                    }  
                    bSuppress = TRUE;  
                    break;  
      case ';'    : bSuppress = TRUE;              
                    break;  
                    
      case V2_TAB : if (pstate)
                      GenByte(BL_PRTOS);
                    pstate = 0;  
                    NumExpression();
                    GenByte(BL_TAB);
                    SynChk(')');
                    break;
                    
      case V2_SPC : if (pstate)
                      GenByte(BL_PRTOS);
                    pstate = 0;  
                    NumExpression();
                    GenByte(BL_SPC);
                    SynChk(')');
                    break;
                    
      default     : UngetChr();
                    if (pstate)
                      GenByte(BL_PRTOS);
                    Expression();              
                    bSuppress = FALSE;
                    pstate = 1;
                    break;
      
    }
  } while ( (c != 0x00) && (c != ':') );
}

/*******************************************************************************
* void GotoGosub(U8 Token)
* same code except the GOTO/GOSUB pcode
*******************************************************************************/

void GotoGosub(U8 Token)
{
  int          Adr,linenum;
  
  linenum = ReadLinenum(TRUE);            // there must be a (valid) linenumber
  Adr = GetLineaddress(linenum);  
  if ( (Pass == 2) && (Adr == 0) )  
    Error(E_UNDEFSTT);
  GenByte(Token);
  GenWord((U16)Adr,HIFIRST);
}

/*******************************************************************************
* void WaitPoke(U8 Token)
* Generate right side of ',' first, then address
*******************************************************************************/

void WaitPoke(U8 Token)
{
  int          i;
  
  GenSetbufmode(CBuff1);                  // reversed order code generation,put 1st in buff
  NumExpression();                        // eval address in buffer
  GenSetbufmode(NULL);                    // back to normal code gen.
  SynChk(',');
  NumExpression();                        // eval mask
  for (i=0; i<CBuff1[0]; i++)            // now add the address-generating code
    GenByte(CBuff1[i+1]);
  GenByte(Token);
}

/*******************************************************************************
* void LoadSaveVerify(U8 Token)
* almost same code except pcode.
* ! SAVE and VERIFY only "name",devnr, LOAD optional SA
*******************************************************************************/

void LoadSaveVerify(U8 Token)
{
  int          limit,i,t,n;
  U8           *pC;
  
  n = 0;
  limit = 2; 
  if (Token == BL_LOAD)
    limit = 3;
  do
  {
    GenSetbufmode(CodeBuffs[n]);
    if (n == 0)
      StringExpression();
    else  
      NumExpression();
    n++;  
  } while ( (TryChr(',')) && (n < limit) );
  if (n < 2)
    Error(E_SYNTAX);                             // must have at least "name",devnum
  t = n;                                         // save nr. arguments
  
  GenSetbufmode(NULL);                   // turn off buffered mode
  while (n >= 0)                          // now push code in reverse order
  {
    pC = CodeBuffs[n];
    for (i=0; i<pC[0]; i++)
      GenByte(pC[1+i]);
    n--;  
  }
  GenByte(Token); GenByte((U8)t);
}

/*******************************************************************************
* void CompEnd(void)                   END
*******************************************************************************/

void CompEnd(void)
{
  GenByte(BL_END);
}

/*******************************************************************************
* void CompFor(void)                   FOR
*******************************************************************************/

void CompFor(void)
{
  VARID        ID;
  U8           pcode;
  U16          name;
  int          type;
  BOOL         bIndexed;
  
  name = ReadVarname(&type,&bIndexed);    // loop var
  if (name == 0)
    Error(E_VAREXP);
  if ( (type == T_STR) || (bIndexed) )
    Error(E_TYPEMISMATCH);                // both int- and floatvars allowed (not in BASIC)
  ID = RefVar(name);                      // enter or get
  SynChk(V2_EQL);
  NumExpression();                        // initial value
  GenStoVar(ID);                          // store initial value into loopvar
  SynChk(V2_TO);
  NumExpression();                        // limit
  pcode = BL_FOR;
  if (TryChr(V2_STEP))
  {
    NumExpression();                         // STEP value
    pcode = BL_FORSTEP;
  }  
  GenPushVarAddress(ID,type,bIndexed);       // address of loopvar
  GenByte(pcode);
}

/*******************************************************************************
* void CompNext(void)                  NEXT
*******************************************************************************/

void CompNext(void)
{
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed;
  BOOL         bMustName;
  
  bMustName = FALSE;
  do
  {
    name = ReadVarname(&type,&bIndexed);       // see if there's a loopvar        
    if (name == 0)
    {
      if (bMustName)
        Error(E_SYNTAX);
      GenByte(BL_NEXT);
    }  
    else  
    {
      if ( (type == T_STR) || (bIndexed) )
        Error(E_TYPEMISMATCH);               // both int- and floatvars allowed (not in BASIC)
      ID = RefVar(name);
      GenPushVarAddress(ID,type,bIndexed);
      GenByte(BL_NEXTVAR);
    }  
    bMustName = TRUE;                     // if ',', must be followed by a loopvar
  } while (TryChr(','));                  // NEXT i,j 
}

/*******************************************************************************
* void CompData(void)                  DATA
*******************************************************************************/

void CompData(void)
{
  int          state;
  int          d,dl;
  U8           c;
  
  state = 0;                              // note 0x16 (DATA) is stored in comp.c, as part of the pcode header
  while (state != 2)
  {
    state = 0;                            // 2 = end of statement, 1 = comma
    d = DataGetIndex();                   // to be patched once length of dataitem is known
    DataGen(0);                           // put a 0 for now
    DataNewitem();                        // add 1 to nr. dataitems
    dl = 0;                               // datalength
    do
    {
      c = GetChr();
      if ( (c == ':') || (c == 0x00) )
        state = 2;
      if (c ==',')  
        state = 1;
      if (state)                          // done this dataitem ?
      {
        DataPatch(d,dl);                  // fill in length
      }
      else
      {
        if (c != 34)                      // don't store quotes
        {
          DataGen(c);                     // to DATA segment
          dl++;
        }
      }  
    } while (state == 0);
  }
  UngetChr();
}

/*******************************************************************************
* void CompInputN(void)                INPUT#
*******************************************************************************/

void CompInputN(void)
{
  NumExpression();                     // channel number
  GenByte(0x48);
  SynChk(',');
  Input();
}

/*******************************************************************************
* void CompInput(void)                 INPUT
*******************************************************************************/

void CompInput(void)
{
  GenByte(0x50);
  Input();
}

/*******************************************************************************
* void CompDim(void)                   DIM
*******************************************************************************/

void CompDim(void)
{
  int          n;
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed,bWarn;
  
  do
  {
    name = ReadVarname(&type,&bIndexed);
    if (name == 0)
      Error(E_VAREXP);
    if (!bIndexed)
      SynChk('(');                          // let SynChk report the error
    n = PushArrayIndices();
    SynChk(')');                            // closing parenthesis
    ID = DefArray(name,n,&bWarn);
    if (bWarn)
      Warn(W_REDIM);
    GenByte(BL_DIM);
    GenByte((U8)n);                         // nr. of dimensions
    GenWord(name,LOFIRST);
    GenWord((U16)ID,HIFIRST);               // array identifiers 7,9,0b,..
  } while (TryChr(','));
}

/*******************************************************************************
* void CompRead(void)                  READ
*******************************************************************************/

void CompRead(void)
{
  int          n;
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed,bWarn;
  
  do
  {
    name = ReadVarname(&type,&bIndexed);               
    if (name == 0)
      Error(E_VAREXP);
    if (bIndexed)                            // array : must push index(es) first
    {
      n = PushArrayIndices();                // generates code
      ID = RefArray(name,n,&bWarn);
      if (bWarn)
      {
        Warn(W_DIMCHANGED);
      }
      SynChk(')');
    }
    else
    {
      ID = RefVar(name);                      // enter or get
    }
    GenPushVarAddress(ID,type,bIndexed);      // push an address
    if (type == T_STR)
      GenByte(BL_READSTR);
    else  
      GenByte(BL_READNUM);
  } while (TryChr(','));
}

/*******************************************************************************
* void CompLet(void)                   LET
*******************************************************************************/

void CompLet(void)
{
  int          t,n;
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed,bWarn;
  BOOL         bTime;
  
  bTime = FALSE;
  name = ReadVarname(&type,&bIndexed);    // get LHS of assignment
  if (name == 0)
    Error(E_VAREXP);
  if (bIndexed)                           // array : must push index(es) first
  {
    n = PushArrayIndices();               // generates code
    ID = RefArray(name,n,&bWarn);
    if (bWarn)
    {
      Warn(W_DIMCHANGED);
    }
    SynChk(')');
  }
  else
  {
    if ( (name == 0x4954) || (name == 0x5453) )  // TI/ST
      Error(E_ILLASSIGN);
    if (name == 0xc954)                          // TI$
      bTime = TRUE;
    else  
      ID = RefVar(name);                        // enter or get
  }
  SynChk(V2_EQL);
  if (type != T_STR)
    type = T_NUM;                             // convert to expression return value
  t = Expression();  
  if (type != t)
    Error(E_TYPEMISMATCH);  
  if ( (bTime) && (t != T_STR) )                 // assign to TI$ must be a string
    Error(E_TYPEMISMATCH);  
  if (bIndexed)
  {
    GenStoArray(ID);                          // store in LHS
  }  
  else  
  {
    if (bTime)
      GenByte(BL_TOTIs);                         // assign to TI$
    else
      GenStoVar(ID);
  }    
}

/*******************************************************************************
* void CompGoto(void)                  GOTO
*******************************************************************************/

void CompGoto(void)
{
  GotoGosub(BL_GOTO);
}

/*******************************************************************************
* void CompRun(void)                   RUN
* CLR, followed by GOTO
*******************************************************************************/

void CompRun(void)
{
  int          Adr,linenum;
  
  GenByte(BL_CLR);
  linenum = ReadLinenum(FALSE);        // optional linenumber
  if (linenum == -1)                   // no linenum ?
    Adr = FindNextline(-1);            // then find first line, skip the CLR, just done it
  else  
    Adr = GetLineaddress(linenum);
  if ( (Pass == 2) && (Adr == 0) )  
    Error(E_UNDEFSTT);
  GenByte(BL_GOTO);
  GenWord((U16)Adr,HIFIRST);
}

/*******************************************************************************
* void CompIf(void)                    IF
*******************************************************************************/

void CompIf(void)
{
  int          patchloc;
  int          Adr,linenum;
  
  NumExpression();                 
  if (TryChr(V2_GOTO))
  {
iflabel1:
    linenum = ReadLinenum(TRUE);
iflabel2:    
    GenByte(BL_IFGOTO);                   // IF...GOTO has own pcode $52 (BL_IFGOTO)
    Adr = GetLineaddress(linenum);  
    if ( (Pass == 2) && (Adr == 0) )  
      Error(E_UNDEFSTT);
    GenWord((U16)Adr,HIFIRST);
    return;
  }
  SynChk(V2_THEN);              
  if (TryChr(V2_GOTO)) goto iflabel1;     // IF..THEN GOTO also pcode $52
  linenum = ReadLinenum(FALSE);           // optional linenr after THEN
  if (linenum != -1)
    goto iflabel2;                        // there's a linenumber (IF..THEN 500). Again, pcode $52
  if (TryChr(V2_RETURN))                  // special case : IF..THEN RETURN -> $58
  {
    GenByte(BL_IFRET);
    return;
  }
  GenByte(BL_IFNOT);                      // No goto, comp. conditional branch
  patchloc = GenGetIndex();               // must compile an offset (to next line)
  GenByte(0);                             // but not yet known (in pass 1)
  if (Pass == 2)
  {
    Adr = FindNextline(curLine);          // lines.c
    Adr -= GetLineaddress(-1);             // patchloc is an offset, make Adr an offset too
    GenPatch(patchloc,(U8)(Adr-patchloc+1));  // fill in 1 byte offset for branch
  }
}

/*******************************************************************************
* void CompRestore(void)               RESTORE
*******************************************************************************/

void CompRestore(void)
{
  GenByte(BL_RESTORE);
}

/*******************************************************************************
* void CompGoSub(void)                 GOSUB
*******************************************************************************/

void CompGosub(void)
{
  GotoGosub(BL_GOSUB);
}

/*******************************************************************************
* void CompReturn(void)                RETURN 
*******************************************************************************/

void CompReturn(void)
{
  GenByte(BL_RETURN);
}

/*******************************************************************************
* void CompRem(void)                   REM
*******************************************************************************/

void CompRem(void)
{
  U8           c;
  U16          name;
  BOOL         bDone;
  int          type;
  BOOL         bIndexed;
  
  do                   
  {
    if (TryString("**FI"))
    {
      do
      {
        bDone = TRUE;
        name = ReadVarname(&type,&bIndexed);
        if ( (name != 0) && (type == T_FLT) && (!bIndexed) )
        {
          AddForcedInt(name); bDone = FALSE;
        }
      } while ( (!bDone) && (TryChr(',')) );
    }
    
    c = GetChr();
  } while (c != 0x00);
  UngetChr();
}

/*******************************************************************************
* void CompStop(void)                  STOP
*******************************************************************************/

void CompStop(void)
{
  GenByte(BL_STOP);
}

/*******************************************************************************
* void CompOn(void)                    ON   GOTO/GOSUB
*******************************************************************************/

void CompOn(void)
{
  int          n;
  int          patchloc,Adr;
  int          linenum;
  U8           pcode;
  
  NumExpression();                        // ON expr.
  if (TryChr(V2_GOTO))
  {
    pcode = BL_ONGOTO;
  }  
  else 
  {
    if (TryChr(V2_GOSUB))
      pcode = BL_ONGOSUB;
    else  
      Error(E_SYNTAX);
  }    
  GenByte(pcode);
  patchloc = GenGetIndex();               // must compile offset to next statement (or line)
  GenByte(0);                             // 1 byte offset
  n = 0;
  do
  {
    linenum = ReadLinenum(TRUE);
    n++; 
    Adr = GetLineaddress(linenum);
    if ( (Pass == 2) && (Adr == 0) )
      Error(E_UNDEFSTT);
    GenWord((U16)Adr,HIFIRST);
  } while (TryChr(','));
   GenPatch(patchloc,(U8)(GenGetIndex()-patchloc+1));
}

/*******************************************************************************
* void CompWait(void)                  WAIT
*******************************************************************************/

void CompWait(void)
{
  WaitPoke(BL_WAIT);
}

/*******************************************************************************
* void CompLoad(void)                  LOAD
*******************************************************************************/

void CompLoad(void)
{
  LoadSaveVerify(BL_LOAD);
}

/*******************************************************************************
* void CompSave(void)                  SAVE
*******************************************************************************/

void CompSave(void)
{
  LoadSaveVerify(BL_SAVE);
}

/*******************************************************************************
* void CompVerify(void)                VERIFY
*******************************************************************************/

void CompVerify(void)
{
  LoadSaveVerify(BL_VERIFY);
}

/*******************************************************************************
* void CompDef(void)                   DEF
*******************************************************************************/

void CompDef(void)
{
  int          t;
  int          patchloc,firstloc;
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed;
  
  SynChk(V2_FN);                          // must be followed  by FN
  name = ReadVarname(&type,&bIndexed);    // reads function name (or error)
  if (name == 0)
    Error(E_VAREXP);
  if ( (type != T_FLT) || (!bIndexed) )   // must be a float AND a '(' must follow the name
    Error(E_SYNTAX);                      
  name = VarMakefunc(name);               // correct hi bits name for func (first hb set, 2nd hb clr)
  ID = DefFunc(name);                     // enter into table
  firstloc = GenGetIndex();
  GenByte(BL_DEFFN);                      
  GenWord((U16)(ID),LOFIRST);             // FN identifier
  GenWord((U16)(ID+7),LOFIRST);           // blitz!.bas says ID (not ID+7)
  patchloc = GenGetIndex();               // Code index, need to patch length byte
  GenByte(0);                             // to be patched . total length (expression code +7)
  name = ReadVarname(&type,&bIndexed);    // read parametername
  if (name == 0)
    Error(E_VAREXP);
  if ( (type != T_FLT) || (bIndexed) )    // must be a float scalar
    Error(E_TYPEMISMATCH);                     
  (void)RefVar(name);                     // don't need ID here
  SynChk(')'); SynChk(V2_EQL);                 
  NumExpression();                        // generate functioncodde
  GenByte(BL_DEFFNEND);                     
  t = GenGetIndex();                        
  GenPatch(patchloc,(U8)(t-firstloc));    // fill in total length
}

/*******************************************************************************
* void CompPoke(void)                  POKE
*******************************************************************************/

void CompPoke(void)
{
  WaitPoke(BL_POKE);
}

/*******************************************************************************
* void CompPrintN(void)                PRINT#
*******************************************************************************/

void CompPrintN(void)
{
  NumExpression();
  if (TryChr(','))
  {
    GenByte(BL_CMD);
    Print(V2_PRINTN);  
  }
  else
  {
    GenByte(BL_PRNCR);
  }
}

/*******************************************************************************
* void CompPrint(void)                 PRINT
*******************************************************************************/

void CompPrint(void)
{
  Print(V2_PRINT);  
}

/*******************************************************************************
* void CompCont(void)                  CONT
*******************************************************************************/

void CompCont(void)
{
  Error(E_ILLCMD);
}

/*******************************************************************************
* void ComList(void)                   LIST
*******************************************************************************/

void CompList(void)
{
  Error(E_ILLCMD);
}

/*******************************************************************************
* void CompClr(void)                   CLR 
*******************************************************************************/

void CompClr(void)
{
  GenByte(BL_CLR);
}

/*******************************************************************************
* void CompCmd(void)                   CMD
*******************************************************************************/

void CompCmd(void)
{
  NumExpression();
  GenByte(BL_CMD);
}

/*******************************************************************************
* void CompSys(void)                   SYS
*******************************************************************************/

void CompSys(void)
{
  NumExpression();                   
  GenByte(BL_SYS); 
  GenByte(BL_PCODE3A);
}

/*******************************************************************************
* void CompOpen(void)                  OPEN
*******************************************************************************/

void CompOpen(void)
{
  int          i,t,n;
  U8           *pC;
  
  n = 0;                                  // arg count. args must be pushed in reverse order,
  do
  {
    GenSetbufmode(CodeBuffs[n]);          // so use buffered Gen   
    if (n < 3)
      NumExpression();                    // code now generated in LFNBuff,...
    else
      StringExpression();
    n++;  
  } while ( (TryChr(',')) && (n < 4) );
  t = n;                                  // save cnt
  n--;
  GenSetbufmode(NULL);                   // turn off buffered mode
  while (n >= 0)                          // now push code in reverse order
  {
    pC = CodeBuffs[n];
    for (i=0; i<pC[0]; i++)
      GenByte(pC[1+i]);
    n--;  
  }
  GenByte(BL_OPEN); GenByte((U8)t);
}

/*******************************************************************************
* void CompClose(void)                 CLOSE
*******************************************************************************/

void CompClose(void)
{
  NumExpression();
  GenByte(BL_CLOSE);
}

/*******************************************************************************
* void CompGet(void)                   GET/GET#
* ! GET# isn't assigned a token in Basic V2
*******************************************************************************/

void CompGet(void)  
{
  BOOL         bGetn;                  // it's GET#
  int          i,n;
  VARID        ID;
  U16          name;
  int          type;
  BOOL         bIndexed,bWarn;
  
  bGetn = FALSE;
  if (TryChr('#'))
  {
    bGetn = TRUE;
    GenSetbufmode(CBuff1);             // Need to repeat #x for each variable
    NumExpression();                   // Generfate it in CBuff1
    GenByte(BL_CMDIN);
    GenSetbufmode(NULL);               // back to normal code gen.
    SynChk(',');                       // there's comma behind GET#x
  }
  do
  {
    name = ReadVarname(&type,&bIndexed);       // see if there's a loopvar        
    if (bIndexed)
    {
      n = PushArrayIndices();
      ID = RefArray(name,n,&bWarn);
      if (bWarn)
      {
        Warn(W_DIMCHANGED);
      }
      SynChk(')');                     // closing parenthesis
    }
    else
    {
      ID = RefVar(name);
    }  
    if (bGetn)
    {
      for (i=0; i<CBuff1[0]; i++)      // Gen the '#x' code
        GenByte(CBuff1[i+1]);
    }
    GenPushVarAddress(ID,type,bIndexed);
    GenByte(BL_GET);
  } while (TryChr(','));    
}

/*******************************************************************************
* void CompNew(void)                   NEW  
*******************************************************************************/

void CompNew(void)
{
  GenByte(BL_NEW);
}

