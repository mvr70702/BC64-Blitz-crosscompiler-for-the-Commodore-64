/*******************************************************************************
* BC.H
*******************************************************************************/

typedef        unsigned char           U8;
typedef        unsigned short          U16;
typedef        int                     BOOL;
typedef        int                     VARID;

#define        FALSE                   0
#define        TRUE                    1

#define        LOFIRST                 0         // byte orer
#define        HIFIRST                 1

// Vartypes. 

#define        T_FLT                   1
#define        T_STR                   2
#define        T_INT                   3

#define        T_NUM                   4         // Expression returns T_STR ot T_NUM, can't tell int or float

// Return values from bc_readnum

#define        N_NONUM                 0
#define        N_INT                   1
#define        N_FLT                   2

/*******************************************************************************
* GLOBALS
*******************************************************************************/

extern         U8      srcFile[0xc000];          // 48 k 

extern         FILE    *fSrc;
extern         FILE    *fOut;
extern         FILE    *fLst;
extern         BOOL    bSkip0;

// bc_comp.c

extern         int      Pass;
extern         int      ProgStart;
extern         U16      curLine;
extern         U8       Pdata[49152];           // the target
extern         int      Compile(void);
extern         U8       GetChr(void);
extern         U8       GetChrAll(void);
extern         void     UngetChr(void);
extern         BOOL     TryChr(U8 tc);
extern         BOOL     TryString(char *pS);
extern         int      GetBufpos(void);
extern         void     List(const char *line,...);
extern         void     ListBuf(char *b);

// bc_vars.c

extern         void     InitVars(void);
extern         U16      ReadVarname(int *pType,BOOL *pIndexed);
extern         U16      VarMakefunc(U16 name);
extern         int      RefVar(U16 name);
extern         int      RefArray(U16 name,int nDims,BOOL *bDimChanged);
extern         int      RefFunc(U16 name);
extern         int      DefArray(U16 name,int nDims,BOOL *bReDim);
extern         int      DefFunc(U16 Name);
extern         void     VarGenArrayInit(int *pIx);
extern         void     VarGenVartab(int *pIx);
extern         void     VarDump(void);
extern         void     CopyVartab(int *pIx);
extern         void     MakeOrgName(U16 name,U8 *pBuff);
extern         void     CreateEmptyvar(int *pIx);
extern         void     AddForcedInt(U16 name);

// bc_errors.c

extern         void     InitErr(void);
extern         int      GetNrErrors(void);
extern         int      GetNrWarnings(void);
extern         void     SynChk(U8 c);
extern         void     Error(int e);
extern         void     Warn(int w);

// bc_comptok.c

extern         void     CompEnd(void);
extern         void     CompFor(void);
extern         void     CompNext(void);
extern         void     CompData(void);
extern         void     CompInputN(void);
extern         void     CompInput(void);
extern         void     CompDim(void);
extern         void     CompRead(void);
extern         void     CompLet(void);
extern         void     CompGoto(void);
extern         void     CompRun(void);
extern         void     CompIf(void);
extern         void     CompRestore(void);
extern         void     CompGosub(void);
extern         void     CompReturn(void);
extern         void     CompRem(void);
extern         void     CompStop(void);
extern         void     CompOn(void);
extern         void     CompWait(void);
extern         void     CompLoad(void);
extern         void     CompSave(void);
extern         void     CompVerify(void);
extern         void     CompDef(void);
extern         void     CompPoke(void);
extern         void     CompPrintN(void);
extern         void     CompPrint(void);
extern         void     CompCont(void);
extern         void     CompList(void);
extern         void     CompClr(void);
extern         void     CompCmd(void);
extern         void     CompSys(void);
extern         void     CompOpen(void);
extern         void     CompClose(void);
extern         void     CompGet(void);
extern         void     CompNew(void);

// bc_genpc.c

extern         void     InitGen(void);
extern         void     GenByte(U8 c);
extern         void     GenWord(U16 w,int Order);
extern         int      GenGetIndex(void);
extern         void     GenPatch(int Index,U8 newbyte);
extern         void     GenSetbufmode(U8 *pBuff);
extern         void     Listcode(int start,int end);
extern         void     GenIntconstBuff(int c,U8 *pBuff,int *n);
extern         void     GenPushIntconst(int c);
extern         void     GenPushFloatconst(U8 *pFloat);
extern         void     GenStoVar(VARID ID);
extern         void     GenStoArray(VARID ID);
extern         void     GenPushVar(VARID ID,int type,BOOL bIndexed);
extern         void     GenPushVarAddress(VARID ID,int type,BOOL bIndexed);
extern         void     CopyCode(int *pIx);

// bc_data.c

extern         void     DataGen(U8 b);
extern         int      DataGetIndex(void);
extern         void     DataNewitem(void);
extern         void     DataGenDataSection(int *pIx);
extern         void     DataDump(void);

// bc_lines.c

extern         void     LinesInit(void);
extern         void     AddLine(int lineNum,int Addr,BOOL bDef);
extern         int      GetLineaddress(int linenum);
extern         void     UpdateLinetable(int CodeAddress);
extern         int      FindNextline(int linenr);
extern         void     LinesDump(void);

// bc_readnum.c

extern         int      ReadNum(int *pInt,U8 *pFlt);

// bc_expr.c

extern         void     MakeCBMFloat(double d,U8 *pCBM);
extern         int      Expression(void);
extern         void     NumExpression(void);
extern         void     StringExpression(void);
