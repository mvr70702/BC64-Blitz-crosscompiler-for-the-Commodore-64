/*******************************************************************************
* Tokensbl.h
* Blitz! pcodes
*******************************************************************************/

#define        BL_GTR                  0x01
#define        BL_EQL                  0x02
#define        BL_GEQ                  0x03
#define        BL_LSS                  0x04
#define        BL_NEQ                  0x05
#define        BL_LEQ                  0x06
#define        BL_ADD                  0x07
#define        BL_SUB                  0x08
#define        BL_MUL                  0x09
#define        BL_DIV                  0x0a
#define        BL_EXPN                 0x0b
#define        BL_AND                  0x0c
#define        BL_OR                   0x0d
#define        BL_UMINUS               0x0e
#define        BL_NOT                  0x0f


#define        BL_DIM                  0x10
#define        BL_FOR                  0x11
#define        BL_FORSTEP              0x12      // FOR with STEP
#define        BL_NEXT                 0x13
#define        BL_NEXTVAR              0x14      // NEXT loopvar            
#define        BL_CLR                  0x15
#define        BL_DATA                 0x16
#define        BL_POKE                 0x17
#define        BL_SYS                  0x18
#define        BL_GOTO                 0x19
#define        BL_GOSUB                0x1a
#define        BL_ONGOTO               0x1b
#define        BL_ONGOSUB              0x1c
#define        BL_RETURN               0x1d
#define        BL_RESTORE              0x1e
#define        BL_IFNOT                0x1f

#define        BL_SGN                  0x20

#define        BL_LEFTs                0x34
#define        BL_RIGHTs               0x35
#define        BL_MIDs                 0x36
#define        BL_DEFFN                0x37
#define        BL_CALLFN               0x38
#define        BL_DEFFNEND             0x39
#define        BL_PCODE3A              0x3a      // always compiled after SYS
#define        BL_PRTAB10              0x3b      // print ,
#define        BL_PRTOS                0x3c      // print TOS;
#define        BL_PRTOSTAB10           0x3d      // print TOS,
#define        BL_PRTOSCR              0x3e      // print TOS <CR>
#define        BL_PRCR                 0x3f      // print <CR>

#define        BL_SPC                  0x40
#define        BL_TAB                  0x41
#define        BL_CMD                  0x42
#define        BL_PRNTOSCR             0x43      // print# TOS <cr>
#define        BL_PRNCR                0x44      // print# <CR>
#define        BL_PRNTOS               0x45      // print# TOS;
#define        BL_GET                  0x46      // GET,GET#
#define        BL_CMDIN                0x48      // redirect input (GET#,INPUT#)
#define        BL_STOP                 0x49
#define        BL_READSTR              0x4a      // READ (DATA)
#define        BL_READNUM              0x4b
#define        BL_WAIT                 0x4c
#define        BL_NEW                  0x4e
#define        BL_END                  0x4f

#define        BL_IFGOTO               0x52
#define        BL_TOTIs                0x56      // assign to TI$
#define        BL_IFRET                0x58      // IF..THEN RETURN
#define        BL_LOAD                 0x5d
#define        BL_SAVE                 0x5e
#define        BL_VERIFY               0x5f

#define        BL_OPEN                 0x60
#define        BL_CLOSE                0x61

#define        BL_PUSHV                0x80

#define        BL_PUSHA                0xa0      // Push address of var xx
#define        BL_PUSHA2               0xa1      // Push address or var xx xx
#define        BL_PUSHARR              0xa4      // 
#define        BL_PUSHARR2             0xa5
#define        BL_PUSHC                0xa6      // Push constant xx
#define        BL_PUSHC2               0xa7      // Push constant hh ll
#define        BL_PUSHFLT              0xa8      // Push 5 byte float constant
#define        BL_PI                   0xaa      // PI
#define        BL_ST                   0xab      // ST var (push)
#define        BL_TI                   0xac      // TI var   ..
#define        BL_TISTR                0xaf      // TI$ var  ..

#define        BL_PUSHCS1              0xb0      // Push constant short 1, bits 0..3 mean 0..15
#define        BL_STOVARQ              0xc0      // store to var 0..31
#define        BL_STOVAR               0xe0      // store to var $20..$ff
#define        BL_STOVAR2              0xe1      // store to var $100+xx to $1ff

#define        BL_STOARRAY             0xe4      // store to array with index 0..255
#define        BL_STOARRAY2            0xe5      // store to array with index > 255
#define        BL_PUSHSTRN             0xe7      // push string constant (length byte following)
#define        BL_PUSHSTR0             0xe8      // push string constant length 0
#define        BL_PUSHSTR1             0xe9      // push string constant length 1
#define        BL_PUSHSTR2             0xea      // push string constant length 2
#define        BL_PUSHSTR3             0xeb      // push string constant length 3
#define        BL_PUSHSTR4             0xed      // push string constant length 4
#define        BL_PUSHSTR5             0xee      // push string constant length 5
#define        BL_PUSHSTR6             0xee      // push string constant length 6
#define        BL_PUSHSTR7             0xef      // push string constant length 7

#define        BL_PUSHCS2              0xf0      // Push constant short 2, bits 0..3 mean 16..31



