/*******************************************************************************
* bc_lines.c
*******************************************************************************/

#include       <stdio.h>
#include       "bc.h" 

/*******************************************************************************
*
*******************************************************************************/

typedef struct
{
  int          Linenr;
  int          Address;
  BOOL         bDefined;
} LINE;

LINE           LineTable[8192];
int            nLines;


/*******************************************************************************
* void LinesInit(void)
*******************************************************************************/

void LinesInit(void)
{
  nLines = 0;
}

/*******************************************************************************
* void AddLine(int lineNum,int Addr,BOOL bDef)
*******************************************************************************/

void AddLine(int lineNum,int Addr,BOOL bDef)
{
  if (Pass == 1)
  {
    LineTable[nLines].Linenr   = lineNum; 
    LineTable[nLines].Address  = Addr;
    LineTable[nLines].bDefined = bDef;
    nLines++;
  };  
}

/*******************************************************************************
* int GetLineaddress(int linenum)
*******************************************************************************/

int GetLineaddress(int linenum)
{
  int          i;
  
  for (i=0; i<nLines; i++)
  {
    if (LineTable[i].Linenr == linenum)
      return LineTable[i].Address;
  }
  return 0;
}

/*******************************************************************************
* void UpdateLinetable(int CodeAddress)
*******************************************************************************/

void UpdateLinetable(int CodeAddress)
{
  int          i;
  
  for (i=0; i<nLines; i++)
    LineTable[i].Address += CodeAddress;
}

/*******************************************************************************
* int FindNextline(int linenr)
* return the address of the line following 'linenr'
*******************************************************************************/

int FindNextline(int linenr)
{
  int          i;
  
  for (i=0; i<nLines; i++)
  {
    if (LineTable[i].Linenr == linenr)
      return (LineTable[i+1].Address);
  }
  return 0;
}

/*******************************************************************************
* void LinesDump(void)
*******************************************************************************/

void LinesDump(void)
{
  int          i;
  
  List("LINETABLE   %d lines\n",nLines);
  List("=================================================================\n");
  for (i=0; i<nLines; i++)
  {
//    List("%5d : $%4X\n",LineTable[i].Linenr,LineTable[i].Address);
    List("%5d : %5d $%4X\n",LineTable[i].Linenr,LineTable[i].Address,LineTable[i].Address);
  }
  ListCR(); 
  ListCR(); 
}  
