/*******************************************************************************
* bc_vars.c
*
* var. names : two characters, like in V2. High bits :
*              0n 0n = FLT
*              8n 8n = INT
*              0n 8n = STRING
*              8n 0n = FN name
*
* All var/array/DEFFN references are by VARID. The runtime creates
* the same variable/array tables as BASIC, and locates vars by 
* those ID's. The VARID is determined by the position in the tables.
*
* Everything is entered in order of appearance in the sourcetext.
*
* DEF FN names are in the same table as scalar names.
* 
* Arrays can be legally redefined (DIM) after issueing a CLR (or other tricks)
* The compiler cannot follow the program flow, so it's impossible to tell what the
* exact number of dimensions should be at any given point, and every number given
* must be considered legal (errors will be caught in runtime)
* To enable the compiler to issue a warning when the dimensioning given doesn't match
* the defined one, or when an array is DIMensioned more than once, the 'RefArray'
* and 'DefArray' functions accept a pointer to a BOOL flag and set/ the flag to TRUE if
* something is 'wrong'.
* 
* 
* Interface :
*
*   U16        ReadVarname(int *pType,BOOL *bIndexed)
*                 Reads 2 char name + '$','%','(' , sets hi bits according to type.
*                 Fills in parameter pointers.
*                 Returns name.
*                 If no name present (first char not alpha), return 0
*
*   U16        VarMakefunc(U16 name)
*                 Changes the hi bits to create a DEFFN varname.
*                 Returns the altered name.
*
*   VARID      RefVar(U16 name)
*                 Call on every occurence of a scalar.
*                 Returns the index in VarTable.
*                  
*   VARID      RefArray(U16 name,int nDims,BOOL *bDimChanged)
*                 Call on every array reference (except DIM).
*                 'nDim' is the number of dimensions as given in the code.
*                 '*bDimChanged' is set to TRUE if nDim doesn't match
*                 the most recent defined nDim.
*                 Returns the index in the ArrayTable.
*
*   VARID      RefFunc(U16 name)
*                 Call on every FN reference.
*                 Returns the index in the VarTable, *but* in Pass 2
*                 -1 is returned if the FN hasn't been defined.
*
*   VARID      DefArray(U16 name,int nDims,BOOL *bReDim)
*                 Call on DIM handling
*                 '*bReDim' is set to TRUE if the array was already defined
*                 or referenced with a different number of dimensions.
*                 Returns the index in ArrayTable.
*
*   VARID      DefFunc(U16 name)
*                 Call on DEF FN handling
*                 Returns the index in VarTable.
*                
* (There's no DefVar, in BASIV V2 scalars are created just by using them).
*******************************************************************************/

#include       <stdio.h>
#include       <ctype.h>
#include       "bc.h" 
#include       "errors.h"
#include       "tokensbl.h"

//TI,TI$,ST,..?

/*******************************************************************************
* The tables
*******************************************************************************/

typedef        struct
{
  U16          vName;
  U16          bDefined;               // for FN names
} VAR;                                 // DEF FN names are also VARs

typedef        struct
{
  U16          vName;
  BOOL         bDefined;               // DIM used
  int          nDims;                  // needed to determine codeproduction 
} ARRAY;


/*******************************************************************************
* VARS
*******************************************************************************/

ARRAY          ArrayTable[1024];
VAR            VarTable[1024];
U16            ForcedInts[256];
int            nForcedInts;

int            nVars;
int            nArrays;
   
static int     GetVarindex(U16 name);
static int     GetArrayindex(U16 name);
static int     EnterVar(U16 name,BOOL bDef);
static int     EnterArray(U16 name,int nDims,BOOL bDef);

/*******************************************************************************
* void InitVars(void)
*******************************************************************************/

void InitVars(void)
{ 
  if (Pass == 1)
  {
    nVars = nArrays = nForcedInts = 0;
  }
}

/*******************************************************************************
* U16 ReadVarname(int *pType,BOOL *pIndexed)
*
* The first character must be an alphabetic, else error
* make sure the inbufptr is on the first char of the name (use UngetChr if
* neccesary)
* low byte = first char, hight bye 2nd
* 
* return 0 if no alpha char found at 1st position
*******************************************************************************/

U16 ReadVarname(int *pType,BOOL *pIndexed)
{
  U8           c;
  U16          name;
  
  c = GetChr();                     // guaranteed to be alphabetic
  if (!(isalpha(c)))
  {
    UngetChr();
    return 0;
  }  
  name = c;  
  c = GetChr();
  if (isalnum(c))
  {
    name |= (c << 8);
    do
    {
      c = GetChr();                    // swallow excess characters in varname
    } while (isalnum(c));
  }
  *pType = T_FLT;
  if (c == '%')
  {
    name |= 0x8080; *pType = T_INT; c = GetChr();
  }  
  if (c == '$')  
  {
    name |= 0x8000; *pType = T_STR; c = GetChr();
  }
  
  *pIndexed = FALSE;
  if (c == '(')
    *pIndexed = TRUE;
  else  
    UngetChr();
    
  // check if this var must be forced to int  
  
  if ( (*pType == T_FLT) && (!(*pIndexed)) && (IsForcedInt(name)) )
  {
    *pType = T_INT;
    name   |= 0x8080;
  }  
  return name;
}

/*******************************************************************************
* U16 VarMakefunc(U16 name)
* set Hi bits name to represent a FN (first bit set, 2nd clr)
*******************************************************************************/

U16 VarMakefunc(U16 name)
{
  return ((name & 0x7fff) | 0x0080);    // clr hi bit n2, set hi bit n1
}

/*******************************************************************************
* VARID RefVar(U16 name)
* Creates a table entry or gets existing
* Call on a reference to a var (not a func) 
* for simple vars, the VARID is the same as the index in the table
*******************************************************************************/

VARID RefVar(U16 name)
{
  int          index;
  
  if ( (name == 0x4954) || (name == 0x5453) )              // TI,ST. Has been checked, just in case
    Error(E_ILLASSIGN);
  index = GetVarindex(name);           // already there ?
  if (index == -1)
    index = EnterVar(name,TRUE);       // no, enter
  return index;  
}

/*******************************************************************************
* VARID RefArray(U16 name,int nDims,BOOL *bDimChanged)
* VARID for arrays = (index*2)+7
*******************************************************************************/

VARID RefArray(U16 name,int nDims,BOOL *bDimChanged)
{
  int          index;
  
  index = GetArrayindex(name);         // already there ?
  if (index == -1)
  {
    index = EnterArray(name,nDims,FALSE);
    *bDimChanged = FALSE;
  }  
  else
  {
    *bDimChanged = (ArrayTable[index].nDims != nDims);
  }
  return (index*2+7);
}

/*******************************************************************************
* int RefFunc(U16 name)
* VARID = index*7+2
*******************************************************************************/

int RefFunc(U16 name)
{
  int          index;
  
  index = GetVarindex(name);
  if (Pass == 2)
  {
    if (VarTable[index].bDefined == FALSE) 
      return -1;
    else  
      return (index*7+2);
  }
  if (index == -1)
    index = EnterVar(name,FALSE);
  return (index*7+2);
}

/*******************************************************************************
* int DefArray(U16 name,int nDims,BOOL *bReDim)
*******************************************************************************/

int DefArray(U16 name,int nDims,BOOL *bReDim)
{  
  int          index;
  
  *bReDim = FALSE;
  index = GetArrayindex(name);
  if (index == -1)
  {
    index = EnterArray(name,nDims,TRUE);
  }
  else
  {
    ArrayTable[index].bDefined = TRUE;
    *bReDim = (nDims != ArrayTable[index].nDims);
  }
  return (index*2+7);
}

/*******************************************************************************
* int DefFunc(U16 name)
*******************************************************************************/

int DefFunc(U16 name)
{
  int          index;
  
  index = GetVarindex(name);
  if (index == -1)
    index = EnterVar(name,TRUE);
  return (index*7+2);
}

/*******************************************************************************
*  void VarGenArrayInit(int *pIx);
*  The 'relocation' DA AA% array and the DIMs' in the ARRAYINFO section
*******************************************************************************/

void VarGenArrayInit(int *pIx)
{
  int          i,j,na,n;
  int          *pSav;
  U8           buff[3];
  ARRAY        *pA;
  
  
  if (nArrays == 0)                    // nothing to do if no arrays
    return;
    
  // build DIM for 'relocation' array
    
  GenIntconstBuff(nArrays-1,buff,&n);
  for (j=0; j<n; j++)
  {
    Pdata[*pIx] = buff[j];             // push the dimension
    (*pIx)++;
  }
  Pdata[*pIx] = BL_DIM; (*pIx)++;
  Pdata[*pIx] = 0x01;   (*pIx)++;      // 1 dimension
  Pdata[*pIx] = 0xDA;   (*pIx)++;      // 'Z'
  Pdata[*pIx] = 0xAA;   (*pIx)++;      // '*'
  Pdata[*pIx] = 0x00;   (*pIx)++;      // this array always has ID 00 00
  Pdata[*pIx] = 0x00;   (*pIx)++;
  
  pA = &ArrayTable[0];
  for (i=0; i<nArrays; i++)  
  {
    if (ArrayTable[i].bDefined == FALSE)                       // generate DIMs for implicit array(s)
    {
      for (j=0; j<pA->nDims; j++)                              // Blitz compiler only accepts 1-dimensional arrays,
      {                                                        // but the runtime handles any nr. dim's.
        Pdata[*pIx] = 0xBA;                                    // not DIMmed, each dimension = 10
        (*pIx)++;                                               
      }  
      Pdata[*pIx] = BL_DIM;                (*pIx)++;
      Pdata[*pIx] = pA->nDims;             (*pIx)++;
      Pdata[*pIx] = pA->vName & 0xff;      (*pIx)++;
      Pdata[*pIx] = (pA->vName>>8) & 0xff; (*pIx)++;
      n = i+i+7;                                               // array ID's are 7,9,..
      Pdata[*pIx] = (n>>8) & 0xff;         (*pIx)++;           // HIFIRST 
      Pdata[*pIx] = (n & 0xff);            (*pIx)++;           // then LO
    }
    pA++;
  }
}

/*******************************************************************************
* int GetVarindex(U16 name)
* Searches VarTable, return -1 if not found
*******************************************************************************/

static int GetVarindex(U16 name)
{
  int          i;
  
  for (i=0; i<nVars; i++)
  {
    if (name == VarTable[i].vName)
      return i;
  }
  return -1;
}

/*******************************************************************************
* int GetArrayindex(U16 name)
* Searches VarTable, return -1 if not found
*******************************************************************************/

static int GetArrayindex(U16 name)
{
  int          i;
  
  i = 0;
  for (i=0; i<nArrays; i++)
  {
    if (name == ArrayTable[i].vName)
      return i;
  }
  return -1;
}

/*******************************************************************************
* int EnterVar(U16 name,BOOL bDef)
*******************************************************************************/

static int EnterVar(U16 name,BOOL bDef)
{
  int          index;
  
  index = GetVarindex(name);
  if ( (Pass == 1) && (index == -1) )
  {
    VarTable[nVars].vName = name;
    VarTable[nVars].bDefined = bDef;       // naming a var == defining it
    index = nVars;
    nVars++;
  }  
  return index;
}

/*******************************************************************************
* void EnterArray(U16 name,int nDims,BOOL bDef)
* Only if Pass ==1 AND doesn't exist yet
* !! array indexes are 7,9,11,..
*******************************************************************************/

static int EnterArray(U16 name,int nDims,BOOL bDef)
{
  int          index;
  
  index = GetArrayindex(name);
  if ( (Pass == 1) && (index == -1) )
  {
    ArrayTable[nArrays].vName    = name;
    ArrayTable[nArrays].bDefined = bDef;
    ArrayTable[nArrays].nDims    = nDims;
    index = nArrays;
    nArrays++;
  }
  return index + index + 7;
}


/*******************************************************************************
* void AddForcedInt(U16 name)
* from the REM **FI instruction; Add the float 'name" to the list of vars
* that will be converted to integer. ReadVarname will then take care of
* the conversion
*******************************************************************************/

void AddForcedInt(U16 name)
{
  if (Pass == 1)
  {
    if (nForcedInts > 255)
      Error(E_FIOVF);
    ForcedInts[nForcedInts] = name;
    nForcedInts++;
  }
}

/*******************************************************************************
* BOOL IsForcedInt(U16 name)
*******************************************************************************/

BOOL IsForcedInt(U16 name)
{
  int          i;
  
  for (i=0; i<nForcedInts; i++)
  {
    if (name == ForcedInts[i])
      return TRUE;
  }
  return FALSE;
}

/*******************************************************************************
* void CopyVartab(int *pIx)
* If nVars == 0, original Blitz! compiler produces 1 var, name 00 00
* That's 7 $00 bytes
*******************************************************************************/

void CopyVartab(int *pIx)
{
  int          i,j;
 
  for (i=0; i<nVars; i++)
  {
    Pdata[*pIx] = VarTable[i].vName & 0xff; (*pIx)++;
    Pdata[*pIx] = (VarTable[i].vName >> 8) & 0xff; (*pIx)++;
    for (j = 0; j<5; j++)
    { 
      Pdata[*pIx] = 0x00; (*pIx)++;
    }
  }
}

/*******************************************************************************
* void CreateEmptyvar(int *pIx)
* rfr to bc_comp.c
*******************************************************************************/

void CreateEmptyvar(int *pIx)
{
  int          j;
  
  if (nVars == 0)
  {
    for (j=0; j<7; j++)
    {
      Pdata[*pIx] = 0x00; (*pIx)++;
    }  
  }
}

/*******************************************************************************
* void MakeOrgName(U16 name,U8 *pBuff)
*******************************************************************************/

void MakeOrgName(U16 name,U8 *pBuff)
{
  U16          mask;
  int          p;
  
  mask = name & 0x8080;                // isolate hi bits
  name = name & 0x7f7f;
  
  if (mask == 0x0080)                  // FN ?
    sprintf(pBuff,"FN");
  else  
    sprintf(pBuff,"  ");

  p = 3;
  sprintf(&pBuff[2],"%c",name & 0x7f);  
  if ( ((name>>8) & 0xff) != 0)
  {
    sprintf(&pBuff[3],"%c",(name >> 8) & 0x7f);  
    p = 4;
  }  
  if (mask == 0x0000)
    pBuff[p] = ' ';
  if (mask == 0x8080)
  {
    pBuff[p] = '%';
//    pBuff[p+1] = '%';                  // list uses varargs
//    p++;
  }  
  if (mask == 0x8000) 
    pBuff[p] = '$';
  if (mask == 0x0080)  
    pBuff[p] = ' ';
//  pBuff[p+1] = 0x00;  
  pBuff[5] = 0x00;
}


/*******************************************************************************
* void VarDump(void)
*******************************************************************************/

void VarDump(void)
{
  int          i,j,index;
  U16          name;
  char         namebuff[8];
  char         buff[32];
  
  List("VARTABLE -- %d Entries\n",nVars);
  List("=================================================================\n");
  List(" Name    ID   def\n\n");
  for (i=0; i<nVars; i++)
  {
    for (j=0; j<8; j++)
      namebuff[j] = 0x20;
    name = VarTable[i].vName;
    MakeOrgName(name,namebuff);
    index = i;
    if ((name & 0x8080) == 0x0080)      // FN ?
      index = i * 7 + 2;
    sprintf(buff,"%s %5d    %d\n",namebuff,index,VarTable[i].bDefined);
    ListBuf(buff);
  }
  ListCR();
  ListCR();
  List("ARRAYTABLE -- %d Entries\n",nArrays);
  List("=================================================================\n");
  List(" Name  Dimd  nDims  ID\n\n");
  for (i=0; i<nArrays; i++)
  {
    for (j=0; j<8; j++)
      namebuff[j] = 0x20;
    name = ArrayTable[i].vName;  
    MakeOrgName(name,namebuff);
    sprintf(buff,"%s   %1d      %d %4d\n",namebuff,ArrayTable[i].bDefined,ArrayTable[i].nDims,i*2+7);
    ListBuf(buff);
  }  
  ListCR();
  List("FORCED INTS -- %d Entries\n",nForcedInts);
  List("=================================================================\n");
  List(" Name\n\n");
  for (i=0; i<nForcedInts; i++)
  {
    for (j=0; j<8; j++)
      namebuff[j] = 0x20;
    name = ForcedInts[i];
    name &= 0x7f7f;
    MakeOrgName(name,namebuff);
    List("%s\n",namebuff);
  }
}
