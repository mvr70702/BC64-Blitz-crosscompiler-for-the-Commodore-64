/*******************************************************************************
* BC_MAIN.C
*
* Blitz! Compiler.
*
* A compiler targeting the BLITZ! runtime
* 
* The compiler accepts a tokenized BASIC V2 program as saved by the
* Commodore 64 (or emulator) and produces a compiled BASIC program.
* 
* bc -inputfile (-Ooutputfile) (-Llistfile) (-G)
*******************************************************************************/

#include       <stdio.h>
#include       <string.h>
#include       <stdlib.h>
#include       "bc.h"


/*******************************************************************************
* VARS
*******************************************************************************/

U8             srcFile[0xc000];        // 48 k 
char           srcName[64];
char           srcBase[64];            // srcname without extension
char           outName[64];
char           lstName[64];
FILE           *fSrc;
FILE           *fOut;
FILE           *fLst;
BOOL           bSkip0;

/*******************************************************************************
* void Header(void)
*******************************************************************************/

void Header(void)
{
  printf("BC64 BLITZ! compiler\n\r");
  printf("by MVR    2024\n\r\n\r");
}

/*******************************************************************************
* void usage(void)
*******************************************************************************/

void usage(void)
{
  printf("BC64 accepts a crunched C64 Basic V2 program and produces a compiled\n\r");
  printf("program for the Blitz! runtime (included)\n\r\n\r");
  printf("bc64 -inputfile (-Ooutputfile) (-Llistfile) -G\n\r\n\r");
  printf("If no output- or listfile given, the defaults <inputbase.cp> and <inputbase.lst> will be used.\n\r");
  printf("option -G leaves $2000..$3fff free for graphics in VIC bank 0.\n\r");
}

/*******************************************************************************
* MAIN
*******************************************************************************/

int main(int argc,char *argv[])
{
  int          i;
  char         *pDot;
  long         fsize;
  long         nread;
  int          nErrors,nWarnings;
  double       d,e;
  U8           cf[5];
  BOOL         bNameset;
  
  bSkip0 = FALSE;
  bNameset = FALSE;
  Header();
  for (i = 1; i <argc; i++)
  {
    if ( (i == 1) && (argv[i][0] != '-') && (argv[i][0] != '/') )
    {
      
      strcpy(srcName,argv[i]);
      if ( (pDot = strrchr(srcName,'.')) != NULL)
        strncpy(srcBase,srcName,(pDot - srcName));
      else  
        strcpy(srcBase,srcName);
      strcpy(outName,srcBase);
      strcat(outName,".cp");
      strcpy(lstName,srcBase);
      strcat(lstName,".lst");
      bNameset = TRUE;
    }  
    else
    {
      if ( (argv[i][0] == '-') || (argv[i][0] == '/') )
      {
        switch (argv[i][1])
        {
          case 'o' :
          case 'O' :
            strcpy(outName,&(argv[i][2]));
            break;
          case 'l' : 
          case 'L' :
            strcpy(lstName,&(argv[i][2]));
            break;
          case '?' :  
            usage();
            return 3;
            break;
          case 'g' :  
          case 'G' :
            bSkip0 = TRUE;
            break;
          default:  
            printf("unknown option %c!!\n\r",argv[i][1]);
            break;
        }
      }
    }
  }  
  if (!bNameset)
  {
    printf("No inputfile given\n");
    return 1;
  }
  printf("Compiling %s    Outfile %s    Lstfile  %s\n\r",srcName,outName,lstName);
    
  // read complete srcfile into 'srcFile'
    
  fSrc = fopen(srcName,"rb");
  if (fSrc == 0)
  {
    printf("!!! Sourcefile %s not found \n\r",srcName);
    return 2;
  }
  fseek(fSrc,0,SEEK_END);
  fsize = ftell(fSrc);
  fseek(fSrc,0,SEEK_SET);
  nread = fread(srcFile,1,fsize,fSrc);
  printf("%s read , %dbytes\n\r",srcName,nread);
  fclose(fSrc);
  
  // remove old outputfile
  
  remove(outName);
  
  // open outputfile
  
  fOut = fopen(outName,"wb");
  if (fOut == 0)
  {
    printf("!!! Cannot open Outputfile %s\n\r",outName);
    return 2;
  }
  
  // open listfile
  
  fLst = fopen(lstName,"w");
  if (fLst == 0)
  {
    printf("!!! Cannot open Listfile %s\n\r",lstName);
    fclose(fOut);
    return 2;
  }
  
  List("Compiling %s    Outfile %s    Lstfile  %s\n\r",srcName,outName,lstName);
  
  // compile
  
  nErrors = Compile();
  nWarnings = GetNrWarnings();
  if (nErrors == 0)
    printf("Compiled, no errors, %d warnings\n",nWarnings);
  else  
    printf("Failed, %d errors, %d warnings\n",nErrors,nWarnings);
  
  return (nErrors==0) ? 0 : 1;
}
