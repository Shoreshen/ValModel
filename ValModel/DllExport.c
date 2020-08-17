#include "Head.h"

extern __declspec(dllexport) int __stdcall ExcelCalc(char *PlanID, int age, int sex, int chnl, char* NP, char* NB, double SA, double Prem, int PayMode, int scen,
	int EffDate, int ValDate, double *Results, int ind)
{
	int       i, norx, ben;
	temp      Temp;
	pols     *Pol;
	resRT    *ResRT;
	resCF    *ResCF, *OutPut;


#pragma region Innitialization
	//Allocate and clear memory
	AllHeap = NULL;
	memset(&GlobAsmp, 0, sizeof(glbAss));
	AllHeap = malloc(sizeof(allHeap));
	Pol     = malloc(sizeof(pols));
	ResRT   = malloc(sizeof(resRT));
	ResCF   = malloc(sizeof(resCF));
	OutPut  = malloc(sizeof(resCF));
	memset(AllHeap,   0, sizeof(allHeap));
	memset(Pol,       0, sizeof(pols));
	memset(ResRT,     0, sizeof(resRT));
	memset(ResCF,     0, sizeof(resCF));
	memset(OutPut,    0, sizeof(resCF));
	memset(Results,   0, TOTLEN * 18 * sizeof(double));
	//Load GlobAsmp
	GlobAsmp.ValDate = ValDate;
	GlobAsmp.ValM    = ValDate % 10000 / 100;
	GlobAsmp.ValY    = ValDate / 10000;
	GlobAsmp.mfac    = (double)1 / (double)NOMperY;
	//Connecting DB
	GlobAsmp.conn = PQconnectdb(CONNSTR);
	if (PQstatus(GlobAsmp.conn) != CONNECTION_OK) { return -1; }
#pragma endregion
	
#pragma region Calculate & fill results
	Pol->EffDate = EffDate;
	AllHeap->PlanID = atoi(PlanID);
	LoadProd(0);
	FillPolicy(0, Pol, NP, NB, "0", chnl, sex, age, PayMode, Prem, SA);
	if (0 == ind) {
		Temp.Dur = 0;
		Temp.DurM = 0;
		CalcRate(0, Pol, ResRT, &Temp, scen);
		for (i = 0; i < Pol->DurED; i++) {
			Results[i]              = ResRT->NodeRT[i].lx;
			Results[i + TOTLEN]     = ResRT->NodeRT[i].qx;
			Results[i + 2 * TOTLEN] = ResRT->NodeRT[i].ix;
			Results[i + 3 * TOTLEN] = ResRT->NodeRT[i].lpx;
			for (norx = 0; norx < AllHeap->Heap.TotNo.NoRx; norx++) {
				Results[i + (4 + norx) * TOTLEN] = ResRT->NodeRT[i].rx[norx];
			}
		}
	}
	else if (1 == ind) {
		Temp.Dur = 0;
		Temp.DurM = 0;
		Temp.PaidPremTerm = 0;
		Temp.LeftPremTerm = Pol->PtrCF.TotPremTerm;
		Temp.SBbegin = 0;
		Temp.SBend = 0;
		CalcCF(0, Pol, ResCF, &Temp);
		for (i = 0; i < Pol->DurED; i++) {
			Results[i]              = ResCF->NodeCF[i].Prem      * Pol->PtrCF.SATimes;
			Results[i + TOTLEN]     = ResCF->NodeCF[i].Comm      * Pol->PtrCF.SATimes;
			Results[i + 2 * TOTLEN] = ResCF->NodeCF[i].CommOR    * Pol->PtrCF.SATimes;
			Results[i + 3 * TOTLEN] = ResCF->NodeCF[i].CircFee   * Pol->PtrCF.SATimes;
			Results[i + 4 * TOTLEN] = ResCF->NodeCF[i].SaftyFund * Pol->PtrCF.SATimes;
			Results[i + 5 * TOTLEN] = ResCF->NodeCF[i].FixExp;
			Results[i + 6 * TOTLEN] = ResCF->NodeCF[i].VarExp    * Pol->PtrCF.SATimes;
			Results[i + 7 * TOTLEN] = ResCF->NodeCF[i].Surr      * Pol->PtrCF.SATimes;
			for (ben = 0; ben < AllHeap->Heap.TotNo.NoBenft; ben++) {
				Results[i + (8 + ben) * TOTLEN] = ResCF->NodeCF[i].Ben[ben] * Pol->PtrCF.SATimes;
			}
		}
	}
	else if (2 == ind) {
		Temp.Dur = 0;
		Temp.DurM = 0;
		CalcRate(0, Pol, ResRT, &Temp, scen);
		Temp.Dur = 0;
		Temp.DurM = 0;
		Temp.PaidPremTerm = 0;
		Temp.LeftPremTerm = Pol->PtrCF.TotPremTerm;
		Temp.SBbegin = 0;
		Temp.SBend = 0;
		CalcCF(0, Pol, ResCF, &Temp);
		CashFlow(0, Pol, ResRT, ResCF, OutPut);
		for (i = 0; i < Pol->PeriodED; i++) {
			Results[i]              = OutPut->NodeCF[i].Prem      * Pol->PtrCF.SATimes;
			Results[i + TOTLEN]     = OutPut->NodeCF[i].Comm      * Pol->PtrCF.SATimes;
			Results[i + 2 * TOTLEN] = OutPut->NodeCF[i].CommOR    * Pol->PtrCF.SATimes;
			Results[i + 3 * TOTLEN] = OutPut->NodeCF[i].CircFee   * Pol->PtrCF.SATimes;
			Results[i + 4 * TOTLEN] = OutPut->NodeCF[i].SaftyFund * Pol->PtrCF.SATimes;
			Results[i + 5 * TOTLEN] = OutPut->NodeCF[i].FixExp;
			Results[i + 6 * TOTLEN] = OutPut->NodeCF[i].VarExp    * Pol->PtrCF.SATimes;
			Results[i + 7 * TOTLEN] = OutPut->NodeCF[i].Surr      * Pol->PtrCF.SATimes;
			for (ben = 0; ben < AllHeap->Heap.TotNo.NoBenft; ben++) {
				Results[i + (8 + ben) * TOTLEN] = OutPut->NodeCF[i].Ben[ben] * Pol->PtrCF.SATimes;
			}
		}
	}
#pragma endregion

#pragma region Free memory & closing files
	FreeProd(0);
	free(AllHeap);
	free(Pol);
	free(ResRT);
	free(ResCF);
	free(OutPut);
	PQfinish(GlobAsmp.conn);
#pragma endregion

	return 0;
}

void CashFlow(int ProdNo, pols *Pol, resRT *ResRT, resCF *ResCF, resCF *OutPut)
{
	int i, dur = Pol->Dur, ben;
	double StartLx = ResRT->NodeRT[dur].lx, *LoopRate;
	if (StartLx > 0) {
		for (i = 0; i < Pol->PeriodED; i++) {
			OutPut->NodeCF[i].Prem      += ResCF->NodeCF[dur].Prem      * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i].Comm      += ResCF->NodeCF[dur].Comm      * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i].CommOR    += ResCF->NodeCF[dur].CommOR    * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i].CircFee   += ResCF->NodeCF[dur].CircFee   * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i].SaftyFund += ResCF->NodeCF[dur].SaftyFund * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i].VarExp    += ResCF->NodeCF[dur].VarExp    * ResRT->NodeRT[dur].lx / StartLx;
			OutPut->NodeCF[i + 1].Surr  += ResCF->NodeCF[dur].Surr      * ResRT->NodeRT[dur].lpx / StartLx;
			OutPut->NodeCF[i].FixExp    += ResCF->NodeCF[dur].FixExp    * ResRT->NodeRT[dur].lx / StartLx * AllHeap[ProdNo].Heap.PAsmp.Infl[i];
			LoopRate = &ResRT->NodeRT[dur].lx;
			for (ben = 0; ben < AllHeap[ProdNo].Heap.TotNo.NoBenft; ben++) {
				OutPut->NodeCF[i].Ben[ben] = ResCF->NodeCF[dur].Ben[ben] * LoopRate[AllHeap[ProdNo].Heap.VAsmp.Benft[ben].RateID] / StartLx;
			}
			dur++;
		}
	}
}