#pragma once
#pragma execution_character_set("utf-8")//Using UTF-8, connecting PostgreSQL

#pragma region Headers include
#include <stdio.h> 
#include <stdlib.h>
#include <windows.h> 
#include <process.h>
#include <libpq-fe.h>
#include <time.h>
#pragma endregion

#pragma region Macro define

#pragma region System
#define NOTHRED 5
#define CONNSTR "host=localhost user=postgres password=45V5FG5h dbname=NIKODB"
//"hostaddr=10.2.11.122 user=Shore password=45V5FG5h dbname=NIKODB"
//"host=localhost user=Shore password=45V5FG5h dbname=NIKODB"
#define MAXILEN 20
#define PRECI   10
#pragma endregion

#pragma region Length
#define TERMLEN    10
#define SQLLEN     1024
#define BUFFLEN    71680//Buffer length 70kb
#pragma endregion

#pragma region Max value
#define NODISC     10
#define NOYEAR     106
#define NOSEX      2
#define NOBEN      10
#define NOBASE     10
#pragma endregion

#pragma region Calculation
#define NORIDER    10
#define NOMperY    12
#define TOTLEN     (NOYEAR * NOMperY + 1)
#define TAILEQPREV 0
#define TAILNA     1
#define FAIL      -1
#define SUCCESS    0
#define SABASE     1000
#pragma endregion

#pragma endregion

#pragma region Structure

#pragma region Node
typedef struct {
	double  qx;
	double  ix;
	double  kx;
	double  rx[NORIDER];
	double  lpx;
} nodesn;
typedef struct {
	double  Prem;
	double  Comm;
	double  CommOR;
	double  CircFee;
	double  SaftyFund;
	double  FixExp;
	double  VarExp;
	double  Surr;
	double  Ben[NOBEN];
} nodecf;
typedef struct {
	double  lx;
	double  qx;
	double  ix;
	double  lpx;
	double  rx[NORIDER];
} nodert;
#pragma endregion

#pragma region Assumption
typedef struct {
	nodesn  NodeSN[NOYEAR];
} sensi;
typedef struct {
	double  qx[NOYEAR];
	double  ix[NOYEAR];
	double  kx[NOYEAR];
	double  rx[NORIDER][NOYEAR];
} rates;
typedef struct {
	int     BenID;
	int     RateID;
	int     NoBase;
	int     BasePos[NOBASE];
	int     CmpInd;
	int     ImprvMthd;
	char    Start[TERMLEN / 2];
	char    End[TERMLEN / 2];
	double  Ratio;
	double  ImprvRT_M;
	double  ImprvRT_Y;
	double  Value;
} benft;
typedef struct {
	int     Chnl;
	char    NP[TERMLEN / 2];
	//Lapse
	double  SkewPre[NOMperY];
	double  SkewPos[NOMperY];
	double  lpxPre[NOYEAR];
	double  lpxPos[NOYEAR];
	double  AdjYE[NOYEAR];
	//Comm
	double  Comm[NOYEAR];
	double  CommOR[NOYEAR];
	//Exp
	int     FYAPE;
	double  FixExp[NOYEAR];
	double  VarExp[NOYEAR];
} varnp;
typedef struct {
	int     Year;
	double  K_Fac;
} kfac;
typedef struct {
	char    NP[TERMLEN / 2];
	double  CV[NOSEX][NOYEAR][NOYEAR + 1];
	double  CVNP[NOSEX][NOYEAR][NOYEAR + 1];
} gcv;
typedef struct {
	int     NoRx;
	int     NoSensi;
	int     NoBenft;
	int     NoVarNP;
	int     NoKfact;
	int     NoGCV;
} tcnt;
typedef struct {
	sensi  *Sensi;
	rates   Rates[NOSEX];
	benft   Benft[NOBEN];
	varnp  *VarNP;
	kfac   *KFact;
	gcv    *GCV;
} vecAss;
typedef struct {
	int     WaitPeriod;
	double  CIRCFee;
	double  SaftyFund;
	double  TPD;
	double  MaxInf;
	double  Infl[TOTLEN];
} pfxAss;
#pragma endregion

#pragma region Policy data
typedef struct {
	//Rates
	rates  *Rates;
	//Lapse
	double *SkewPre;
	double *SkewPos;
	double *lpxPre;
	double *lpxPos;
	double *AdjYE;
} ptrRate;
typedef struct {
	int     PayMode;
	int     PayTerm;
	int     TotPremTerm;
	int     BenDurST[NOBEN];
	int     BenDurED[NOBEN];
	double  SATimes;
	double  PremAnnl;
	double  PremTerm;
	//Comm
	double *Comm;
	double *CommOR;
	//Exp
	int     FYAPE;
	double *FixExp;
	double *VarExp;
	//GCV
	double *CV;
	double *CVNP;
} ptrCF;
typedef struct {
	int     Error;
	int     ReCalcRT;
	int     ReCalcCF;
	int     AgentPos;
	int     Count;
	int     Agent;
	int     EffDate;
	int     issAge;
	int     Dur;
	int     BenEndAge;
	int     PayEndAge;
	int     PeriodED;
	int     DurED;
	ptrRate PtrRate;
	ptrCF   PtrCF;
} pols;
#pragma endregion

#pragma region Calculation
typedef struct {
	int     DurM;
	int     Dur;
	int     LeftPremTerm;
	int     PaidPremTerm;
	double  SBbegin;
	double  SBend;
} temp;
typedef struct {
	double  lx;
	double  qx;
	double  ix;
	double  rx[NORIDER];
	double  lpx;
} tempRT;
#pragma endregion

#pragma region Results
typedef struct {
	nodecf  NodeCF[TOTLEN];
}resCF;
typedef struct {
	nodert  NodeRT[TOTLEN];
}resRT;
typedef struct {
	int     Agent;
	int     Sensi;
	int     K_Need;
	double  Prev[TOTLEN];
	double  Curr[TOTLEN];
	double  K[TOTLEN];
	resCF   CF;
	resCF   CF_K;
}agtRes;
#pragma endregion

#pragma region Heap & Threads
typedef struct {
	int     NoPol;
	int     NoAgn;
	tcnt    TotNo;
	pfxAss  PAsmp;
	vecAss  VAsmp;
	pols   *Pols;
	agtRes *AgentRes;
}heap;
typedef struct {
	int     ProdNo;
	int     StartNo;
	int     EndNo;
} tInfo;
#pragma endregion

#pragma region Global structure
typedef struct {
	int     LoadAllHeapFail;
	PGconn *conn;
	int     ValDate;
	int     ValMon;
	int     ValM;
	int     ValY;
	int     NoProd;
	int     NoDisc;
	double  mfac;
	double  AccRate[NODISC][TOTLEN];
} glbAss;
typedef struct {
	tInfo   TInfo[NOTHRED];
	heap    Heap; 
	int     PlanID;
	int		TotNoalloc;
	int     MaxAgntPos;
	int     Loaded;
	int     Printed;
}allHeap;
#pragma endregion

#pragma endregion

#pragma region Function

#pragma region DBFunc
int DBConn(PGconn **conn, char *ConnStr, FILE *fp);
int DBExec(PGconn *conn, PGresult **res, char *sql, FILE *fp);
#pragma endregion

#pragma region CharFunc
void ParseArray(double* Rate, int Dsize, char* Sstr, int tail);
int ParseArrayInt(int* Rate, int Dsize, char* Sstr, int tail);
void AsmbCF(char *buffer, int size, int ProdNo, int i, int Pos, int LineEnd);
__inline char* dbltoa(char* buff, double A, int Precision);
#pragma endregion

#pragma region DateFunc
int DaysOfMonth(int year, int month);
#pragma endregion

#pragma region LoadAsump
int LoadGlable(void);
void ThreadLoadProd(LPVOID pM);
void LoadProd(int i);
void LoadPolicys(int i);
inline void FillPolicy(int i, pols *Pols, char *NP, char *NB, char *PolNo, int Chnl, int Sex, int issAge, int PayMode, double Prem, double SA);
void FreeProd(int i);
#pragma endregion

#pragma region DllExport
extern __declspec(dllexport) int __stdcall ExcelCalc(char *PlanID, int age, int sex, int chnl, char* NP, char* NB, double SA, double Prem, int PayMode, int scen,
	int EffDate, int ValDate, double *Results, int ind);
void CashFlow(int ProdNo, pols *Pol, resRT *ResRT, resCF *ResCF, resCF *OutPut);
#pragma endregion

#pragma region CalcFunc
__inline void CalcRate(int ProdNo, pols *Pol, resRT *ResRT, temp *Temp, int sensi);
__inline void CalcCF(int ProdNo, pols *Pol, resCF *ResCF, temp *Temp);
__inline void CalcBen(int ProdNo, pols *Pol, resCF *ResCF, temp *Temp, int ben, int Age);
void ThreadCalc(LPVOID pM);
#pragma endregion

#pragma region OutPut
void ThreadOutP(LPVOID pM);
#pragma endregion

#pragma endregion

#pragma region Global variables
extern glbAss    GlobAsmp;
extern allHeap  *AllHeap;
extern FILE     *fpAsmp, *fpOutP, *fpMain;
extern char      NumRep[11];
#pragma endregion
