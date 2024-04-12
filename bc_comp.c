/*******************************************************************************
* BC_COMP.C
*
* The file generated consists of 2 blocks :
*   1) the runtime        Pcode interpreter + runtime lib
*   2) the PDATA block    - 6 pointers into the PDATA
*                         - an Array header + descriptors if there are implicit arrays (Not DIM'd)
*                         - DATA items, in the format len,data,len,.., if there are DATA/READ statements
*                         - the pcode instructions
*                         - the static vartable for scalars,
*******************************************************************************/

#include       <stdio.h>
#include       <stdlib.h>
#include       <stdarg.h>
#include       <setjmp.h>
#include       <time.h>
#include       "bc.h"
#include       "errors.h"
#include       "tokensbl.h"
#include       "tokensV2.h"

/*******************************************************************************
* Memory reserved for Pdata
*******************************************************************************/

U8             Pdata[49152];           // the target
int            PdataStart;             // normally $1f93
int            PdataIx;
int            ArrayinfoStart;
int            VartabStart;
int            VartabEnd;
int            DataStart;
int            ProgStart;

/*******************************************************************************
* VARS
*******************************************************************************/
                    
U8             *pSrc;       
U8             Inbuf[80];
U8             *pInbuf;
int            Pass;
U16            curLine;                // linenr. being compiled
jmp_buf        JB;                     // for errorhandling

/*******************************************************************************
* PtrNames
*******************************************************************************/

U8             *PtrNames[] = {
  "Vartab start  ",
  "Vartab end    ",
  "Arraytab start",
  "Array Info    ",
  "Data start    ",
  "Pcode start   ",
};

/*******************************************************************************
* Dispatch list for V2 tokens $80..$A2 (statement starters)
*******************************************************************************/

void (*dispatch[])(void) = {
  &CompEnd,                            // $80
  &CompFor,
  &CompNext,
  &CompData,
  &CompInputN,
  &CompInput,
  &CompDim,
  &CompRead,
  &CompLet,                            // $88
  &CompGoto,
  &CompRun,
  &CompIf,
  &CompRestore,
  &CompGosub,
  &CompReturn,
  &CompRem,
  &CompStop,                           // $90
  &CompOn,
  &CompWait,
  &CompLoad,
  &CompSave,
  &CompVerify,
  &CompDef,
  &CompPoke,
  &CompPrintN,                         // $98
  &CompPrint,
  &CompCont,
  &CompList,
  &CompClr,
  &CompCmd,
  &CompSys,
  &CompOpen,
  &CompClose,                          // $a0
  &CompGet,
  &CompNew,                            // $a2
  
};


/*******************************************************************************
* void List(const char *line,...)
*******************************************************************************/

void List(const char *line,...)
{
  va_list args;
  char    buffer[128];

  va_start(args,line);
  vsprintf(buffer,line,args);
  va_end(args);
  fprintf(fLst,buffer);
}

/*******************************************************************************
* void ListBuf(char *b)
*******************************************************************************/

void ListBuf(char *b)
{
  while (*b)
  {
    fputc(*b,fLst);
    b++;
  }
}

/*******************************************************************************
* void ListCR(void)
*******************************************************************************/

void ListCR(void)
{
  List("\n");
}

/*******************************************************************************
* void ReadLine(void)
* to Inbuf, drop link/linenum
*******************************************************************************/

BOOL ReadLine(void)
{
  int          i;
                
  pInbuf = &Inbuf[0];  
  
  if ( (pSrc[0] == 0) && (pSrc[1] == 0) )        // EOF ?
    return 0;
    
  curLine = (pSrc[2] + 256 * pSrc[3]);
  if (curLine > 63999)
    Error(E_LINENUM);
  pSrc += 4;
  while (*(pInbuf++) = *(pSrc++));               // get line until 0x00
  pInbuf = &Inbuf[0];
  return 1;
}

/*******************************************************************************
* U8 GetChr(void)
*******************************************************************************/

U8 GetChr(void)
{
  U8           c;
  
  do
  {
    c = *pInbuf;
    pInbuf++;
  } while (c == 0x20);
  return c;
}

/*******************************************************************************
* U8 GetChrAll(void)
*******************************************************************************/

U8 GetChrAll(void)
{
  U8           c;
  
  c = *pInbuf;
  pInbuf++;
  return c;
}

/*******************************************************************************
* void UngetChr(void)
*******************************************************************************/

void UngetChr(void)
{
  pInbuf--;
}

/*******************************************************************************
* BOOL TryChr(U8 tc)
*******************************************************************************/

BOOL TryChr(U8 tc)
{
  U8           c;
  
  c = GetChr();
  if (c != tc)
    UngetChr();
  return (c == tc);  
}

/*******************************************************************************
* BOOL TryString(char *pS)
* retain pInbuf if no match !
*******************************************************************************/

BOOL TryString(char *pS)
{
  U8           *pSav;
  U8           c;
  
  pSav = pInbuf;
  while (*pS != 0)
  {
    c = GetChr();
    if (*pS != c)
    {
      pInbuf = pSav;
      return FALSE;
    }
    else
    {
      pS++; 
    }
  }
  return TRUE;
}

/*******************************************************************************
* int GetBufpos(void)
*******************************************************************************/

int GetBufpos(void)
{
  return (pInbuf - &Inbuf[0]);
}

/*******************************************************************************
* void CompToken(U8 Tok)
* Dispatch to bc_comptok
*******************************************************************************/

void CompToken(U8 Tok)
{
  if (Tok <= 0xa2)
    (*dispatch[Tok-0x80])();
  else  
    Error(E_UNKNOWN);
}

/*******************************************************************************
* void CompileLine(U16 LineNum)
*******************************************************************************/

void CompileLine(U16 LineNum)
{   
  U8           c;
  U8           var[2];
  int          coloncnt;
  
  GenSetbufmode(NULL);                 // make sure bufmode is off at start of line
  AddLine(LineNum,(U16)GenGetIndex(),TRUE);    
  printf(".");
  do
  {
    coloncnt = 0;
    c = GetChr(); 
    while (c == ':')
    {         
      coloncnt++;
      if (coloncnt >= 2)               // BASIC extension is preceeded by at least 2 colons
              ;
      c = GetChr();        
    }
    
    if (c == 0x00)                     // EOL ?
      return; 
      
    if (c & 0x80)                      // token ?
    {
      CompToken(c);
    }  
    else
    {
      UngetChr();                      // if first char of varname, unget for 'ReadVarname()'
      CompToken(V2_LET);               // no, must be a var (LET code will signal error if not)
    }  
      
    c = GetChr();                      // must swallow ':' after statement for extensions
    if (c != ':')
      UngetChr();
  } while (c != 0x00);
}  

/*******************************************************************************
* BOOL LoadRtlib(void)
*******************************************************************************/

BOOL LoadRtlib(void)
{
  FILE         *fRtlib;
  size_t       fsize;
  size_t       nread;
  
  fRtlib = fopen("rtlib.bin","rb");
  if (fRtlib == 0)
  {
    printf("!!! Cannot open rtlib.bin\n\r");
    return FALSE;
  }
  fseek(fRtlib,0,SEEK_END);
  fsize = ftell(fRtlib);
  fseek(fRtlib,0,SEEK_SET);
  nread = fread(&Pdata[0x7ff],1,fsize,fRtlib);  // put the 0801 commodore file header where it belongs
  fclose(fRtlib);
  printf("rtlib.bin loaded, read %d bytes from %d\n\r",nread,fsize);
  printf("Progheader starts at %04x\n\r",(0x801+nread-2));
  PdataStart = (0x801+nread-2);           // start generating compiled prog at this loc
  return TRUE;
}

/*******************************************************************************
* int Compile(void)
*******************************************************************************/

int Compile(void)
{
  U8           c;
  int          rv;
  int          start,end;
  time_t       rawtime;
  char         tt[64];
  int          i;
  
  time(&rawtime);
  strcpy(tt,ctime(&rawtime));
  ListBuf(tt);
  
  // Read the runtime+interpreter to Pdata[]; no use going on if it can't be read
  
  if (!(LoadRtlib()))
  {  
    printf("Cannot load rtlib.s\n\r");
    return 2;
  }  
  
  // PASS 1
  
  printf("Pass 1\n");
  Pass = 1;
  InitVars();
  InitGen();                           // start 
  InitErr();
  DataInit();
  LinesInit();
  start = GenGetIndex();
  AddLine(-1,start,TRUE);              // artificial line -1 for the CLR instruction
  GenByte(BL_CLR);                     // always starts program
  pSrc = &srcFile[2];                  // reset srcptr, skip fileheader
  rv = setjmp(JB);                     // Error() returns2 here for next line
  while (ReadLine())
  {
    CompileLine(curLine);              // COMPILE
  } 
  AddLine(64000,GenGetIndex(),TRUE);   // Artificial line for the END, also needed for the IF statement,
  GenByte(BL_END);
  ListCR();
  printf("\n\r");
  DataDump();
  
  // now everything needed to determine actual addresses is known
  
  PdataIx = PdataStart + 12;           // reserve room for 6 pointers
  
  // Overrides for starting the pcode at other addresses here, e.g PDataIx = $4000
  
  if (bSkip0)
    PdataIx = 0x4000;
  ArrayinfoStart = PdataIx;
  VarGenArrayInit(&PdataIx);           // generates array info, updates PdataIx
  
  Pdata[PdataIx] = 0x16;
  PdataIx++;
  
  DataStart = PdataIx;
  DataGenDataSection(&PdataIx);        // add the DATA items, update PdataIx
  Pdata[PdataIx] = 0xFF;         
  PdataIx++;                           // pDataIx now has start of Pcode for program 
  
  UpdateLinetable(PdataIx);            // set all line addresses to runtime value
  LinesDump();
  
  // Could stop here if there are errors, but do pass 2 anyway to produce a
  // (hopefully informative) listfile
  
  // PASS 2
  
  printf("Pass 2\n\r");
  
  pSrc = &srcFile[2];                  // reset srcptr, skip fileheader
  Pass = 2;
  InitGen();
  ProgStart = PdataIx;
  start = GenGetIndex();
  GenByte(BL_CLR);                     // always starts program
  end   = GenGetIndex();
  curLine = 0;                         // list the CLR
  List("PCODE\n");
  List("=================================================================\n");
  Listcode(start,end);
  rv = setjmp(JB);                     // Error() returns here for next line
  while (ReadLine())
  {
    start = GenGetIndex();
    CompileLine(curLine);              // COMPILE
    end   = GenGetIndex();
    Listcode(start,end);
  } 
  start = GenGetIndex();
  GenByte(BL_END);                     // always starts program
  end   = GenGetIndex();
  curLine = 64000;                     // list the END
  Listcode(start,end);
  ListCR();
  ListCR(); 

  CopyCode(&PdataIx);               // add code, update PdataIx
  
  VartabStart = PdataIx;
  CopyVartab(&PdataIx);              // add the vartable
  
  VartabEnd = PdataIx;
  
  if (VartabEnd >= 0xa000)
    Error(E_MEMFULL);
  
  // write the 6 pointers
  
  *(U16 *)(&Pdata[PdataStart+0])  = (U16)VartabStart;
  *(U16 *)(&Pdata[PdataStart+2])  = (U16)VartabEnd;
  *(U16 *)(&Pdata[PdataStart+4])  = (U16)VartabEnd;
  *(U16 *)(&Pdata[PdataStart+6])  = (U16)ArrayinfoStart;
  *(U16 *)(&Pdata[PdataStart+8])  = (U16)DataStart;
  *(U16 *)(&Pdata[PdataStart+10]) = (U16)ProgStart;
  
  // print the pointers
  
  List("HEADER\n");
  List("=================================================================\n");
  for (i=0; i<6; i++)
  {
    List("%s :  %04X\n",PtrNames[i],*(U16*)&Pdata[PdataStart+i*2]);
  }
  ListCR();
  ListCR(); 
  
  // !! if the vartable is empty, the original Blitz! compiler adds 7 $00 bytes to the end of
  // the program, but doesn't count them in the program length, and doesn't adjust pointers 2 & 3
  // to keep the output completely compatible, do the same.
  
  CreateEmptyvar(&PdataIx);
  
  VarDump();
  
  // If no errors, write the output file
  
  if ((i=GetNrErrors()) == 0)
  {
    fwrite(&Pdata[0x7ff],1,(PdataIx-0x7ff),fOut);
    fclose(fOut);
    printf("\nOutputfile written\n");
  }  
  else
  {
    printf("\nFailed with %d Error(s)\n",i);
  }
  fclose(fLst);
  printf("\nListfile written\n");
  return i;                            // nrerrors
}

