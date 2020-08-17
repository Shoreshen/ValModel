#include "Head.h"

__inline void CalcRate(int ProdNo, pols *Pol, resRT *ResRT, temp *Temp, int sensi)
{
	int     Age, j, norx;
	double  TotDec;
	tempRT  TempRT;

	ResRT->NodeRT[0].lx = 1;

#pragma region Pre-payment period
	for (Age = Pol->issAge; Age < Pol->PayEndAge; Age++) {
		//Calculate rates
		TempRT.qx = Pol->PtrRate.Rates->qx[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].qx * (1 - Pol->PtrRate.Rates->kx[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].kx);
		if (TempRT.qx > 1) { TempRT.qx = 1; }
		else if (TempRT.qx < 0) { TempRT.qx = 0; }
		TempRT.ix = Pol->PtrRate.Rates->ix[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].ix;
		if (TempRT.ix > 1) { TempRT.ix = 1; }
		else if (TempRT.ix < 0) { TempRT.ix = 0; }
		TotDec = 1 - TempRT.qx - TempRT.ix;
		if (TotDec > 1) { TotDec = 1; }
		else if (TotDec < 0) { TotDec = 0; }
		TotDec = (1 - pow(TotDec, GlobAsmp.mfac)) / (TempRT.qx + TempRT.ix);
		TempRT.qx = TotDec * TempRT.qx;
		TempRT.ix = TotDec * TempRT.ix;
		for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
			TempRT.rx[norx] = 1 - pow(1 - Pol->PtrRate.Rates->rx[norx][Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].rx[norx], GlobAsmp.mfac);
		}
		TempRT.lpx = Pol->PtrRate.lpxPre[Temp->Dur] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].lpx;
		//Calculate month 1-11
		if (TempRT.lpx > 1) { TempRT.lpx = 1; }
		else if (TempRT.lpx < 0) { TempRT.lpx = 0; }
		for (j = 0; j < NOMperY - 1; j++) {
			ResRT->NodeRT[Temp->DurM].qx = ResRT->NodeRT[Temp->DurM].lx * TempRT.qx;
			ResRT->NodeRT[Temp->DurM].ix = ResRT->NodeRT[Temp->DurM].lx * TempRT.ix;
			for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
				ResRT->NodeRT[Temp->DurM].rx[norx] = ResRT->NodeRT[Temp->DurM].lx * TempRT.rx[norx];
			}
			ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM].lx - ResRT->NodeRT[Temp->DurM].qx - ResRT->NodeRT[Temp->DurM].ix;
			ResRT->NodeRT[Temp->DurM].lpx = ResRT->NodeRT[Temp->DurM + 1].lx * (1 - pow(1 - TempRT.lpx, Pol->PtrRate.SkewPre[j]));
			ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM + 1].lx - ResRT->NodeRT[Temp->DurM].lpx;
			Temp->DurM++;
		}
		//Calculate month 12: end of year
		TempRT.lpx = 1 - pow(1 - TempRT.lpx, Pol->PtrRate.SkewPre[j]);
		TempRT.lpx += Pol->PtrRate.AdjYE[Temp->Dur] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].lpx;
		if (TempRT.lpx > 1) { TempRT.lpx = 1; }
		else if (TempRT.lpx < 0) { TempRT.lpx = 0; }
		ResRT->NodeRT[Temp->DurM].qx = ResRT->NodeRT[Temp->DurM].lx * TempRT.qx;
		ResRT->NodeRT[Temp->DurM].ix = ResRT->NodeRT[Temp->DurM].lx * TempRT.ix;
		for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
			ResRT->NodeRT[Temp->DurM].rx[norx] = ResRT->NodeRT[Temp->DurM].lx * TempRT.rx[norx];
		}
		ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM].lx - ResRT->NodeRT[Temp->DurM].qx - ResRT->NodeRT[Temp->DurM].ix;
		ResRT->NodeRT[Temp->DurM].lpx = ResRT->NodeRT[Temp->DurM + 1].lx * TempRT.lpx;
		ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM + 1].lx - ResRT->NodeRT[Temp->DurM].lpx;
		Temp->DurM++;
		//Next loop
		Temp->Dur++;
	}
#pragma endregion

#pragma region Post payment period
	for (Age = Pol->PayEndAge; Age < Pol->BenEndAge; Age++) {
		//Calculate rates
		TempRT.qx = Pol->PtrRate.Rates->qx[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].qx;
		if (Age == NOYEAR - 1) { TempRT.qx = 1; }
		TempRT.qx = TempRT.qx * (1 - Pol->PtrRate.Rates->kx[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].kx);
		if (TempRT.qx > 1) { TempRT.qx = 1; }
		else if (TempRT.qx < 0) { TempRT.qx = 0; }
		TempRT.ix = Pol->PtrRate.Rates->ix[Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].ix;
		if (TempRT.ix > 1) { TempRT.ix = 1; }
		else if (TempRT.ix < 0) { TempRT.ix = 0; }
		TotDec = 1 - TempRT.qx - TempRT.ix;
		if (TotDec > 1) { TotDec = 1; }
		else if (TotDec < 0) { TotDec = 0; }
		TotDec = (1 - pow(TotDec, GlobAsmp.mfac)) / (TempRT.qx + TempRT.ix);
		TempRT.qx = TotDec * TempRT.qx;
		TempRT.ix = TotDec * TempRT.ix;
		for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
			TempRT.rx[norx] = 1 - pow(1 - Pol->PtrRate.Rates->rx[norx][Age] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].rx[norx], GlobAsmp.mfac);
		}
		TempRT.lpx = Pol->PtrRate.lpxPos[Temp->Dur] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].lpx;
		if (TempRT.lpx > 1) { TempRT.lpx = 1; }
		else if (TempRT.lpx < 0) { TempRT.lpx = 0; }
		//Calculate month 1-11
		for (j = 0; j < NOMperY - 1; j++) {
			ResRT->NodeRT[Temp->DurM].qx = ResRT->NodeRT[Temp->DurM].lx * TempRT.qx;
			ResRT->NodeRT[Temp->DurM].ix = ResRT->NodeRT[Temp->DurM].lx * TempRT.ix;
			for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
				ResRT->NodeRT[Temp->DurM].rx[norx] = ResRT->NodeRT[Temp->DurM].lx * TempRT.rx[norx];
			}
			ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM].lx - ResRT->NodeRT[Temp->DurM].qx - ResRT->NodeRT[Temp->DurM].ix;
			ResRT->NodeRT[Temp->DurM].lpx = ResRT->NodeRT[Temp->DurM + 1].lx * (1 - pow(1 - TempRT.lpx, Pol->PtrRate.SkewPos[j]));
			ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM + 1].lx - ResRT->NodeRT[Temp->DurM].lpx;
			Temp->DurM++;
		}
		//Calculate month 12: end of year
		TempRT.lpx = 1 - pow(1 - TempRT.lpx, Pol->PtrRate.SkewPos[j]);
		TempRT.lpx += Pol->PtrRate.AdjYE[Temp->Dur] * AllHeap[ProdNo].Heap.VAsmp.Sensi[sensi].NodeSN[Temp->Dur].lpx;
		if (TempRT.lpx > 1) { TempRT.lpx = 1; }
		else if (TempRT.lpx < 0) { TempRT.lpx = 0; }
		ResRT->NodeRT[Temp->DurM].qx = ResRT->NodeRT[Temp->DurM].lx * TempRT.qx;
		ResRT->NodeRT[Temp->DurM].ix = ResRT->NodeRT[Temp->DurM].lx * TempRT.ix;
		for (norx = 0; norx < AllHeap[ProdNo].Heap.TotNo.NoRx; norx++) {
			ResRT->NodeRT[Temp->DurM].rx[norx] = ResRT->NodeRT[Temp->DurM].lx * TempRT.rx[norx];
		}
		ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM].lx - ResRT->NodeRT[Temp->DurM].qx - ResRT->NodeRT[Temp->DurM].ix;
		ResRT->NodeRT[Temp->DurM].lpx = ResRT->NodeRT[Temp->DurM + 1].lx * TempRT.lpx;
		ResRT->NodeRT[Temp->DurM + 1].lx = ResRT->NodeRT[Temp->DurM + 1].lx - ResRT->NodeRT[Temp->DurM].lpx;
		Temp->DurM++;
		//Next loop
		Temp->Dur++;
	}
#pragma endregion

}

__inline void CalcCF(int ProdNo, pols *Pol, resCF *ResCF, temp *Temp)
{
	int Age, j, ben;
	double s, m, mk, CIRC = Pol->PtrCF.PremTerm * AllHeap[ProdNo].Heap.PAsmp.CIRCFee, SftFd = Pol->PtrCF.PremTerm * AllHeap[ProdNo].Heap.PAsmp.SaftyFund;

#pragma region Pre-payment period
	for (Age = Pol->issAge; Age < Pol->PayEndAge + 1; Age++) {
		mk = 0;
		for (j = 0; j < NOMperY; j++) {
			//Fix expense
			ResCF->NodeCF[Temp->DurM].FixExp = Pol->PtrCF.FixExp[Temp->Dur];
			//Prem related
			if (j % Pol->PtrCF.PayTerm == 0) {
				mk++;
				Temp->PaidPremTerm++;
				Temp->LeftPremTerm--;
				ResCF->NodeCF[Temp->DurM].Prem = Pol->PtrCF.PremTerm;
				ResCF->NodeCF[Temp->DurM].Comm = Pol->PtrCF.PremTerm * Pol->PtrCF.Comm[Temp->Dur];
				ResCF->NodeCF[Temp->DurM].CommOR = ResCF->NodeCF[Temp->DurM].Comm * Pol->PtrCF.CommOR[Temp->Dur];
				ResCF->NodeCF[Temp->DurM].CircFee = CIRC;
				ResCF->NodeCF[Temp->DurM].SaftyFund = SftFd;
				ResCF->NodeCF[Temp->DurM].VarExp = Pol->PtrCF.PremTerm * Pol->PtrCF.VarExp[Temp->Dur];
			}
			//Surrander benefit
			s = GlobAsmp.mfac * (j + 1);
			m = mk / Pol->PtrCF.PayMode;
			ResCF->NodeCF[Temp->DurM].Surr = ((1 - s) * (Pol->PtrCF.CV[Temp->Dur] - Temp->SBbegin) +
				s * (Pol->PtrCF.CV[Temp->Dur + 1] + Temp->SBend) +
				(m - s) * Pol->PtrCF.CVNP[Temp->Dur + 1]);
			//Other listed benefit
			for (ben = 0; ben < AllHeap[ProdNo].Heap.TotNo.NoBenft; ben++) {
				CalcBen(ProdNo, Pol, ResCF, Temp, ben, Age);
			}
			Temp->DurM++;
		}
		Temp->Dur++;
	}
#pragma endregion

	//Adjust first year APE
	if (Pol->PtrCF.FYAPE) { 
		ResCF->NodeCF[0].VarExp = ResCF->NodeCF[0].VarExp * min(1, 0.1 *	(double)(Pol->PayEndAge - Pol->issAge));
	}

#pragma region Pos-payment period
	for (Age = Pol->PayEndAge + 1; Age < Pol->BenEndAge; Age++) {
		for (j = 0; j < NOMperY; j++) {
			//Fix expense
			ResCF->NodeCF[Temp->DurM].FixExp = Pol->PtrCF.FixExp[Temp->Dur];
			//Surrander
			s = GlobAsmp.mfac * (j + 1);
			ResCF->NodeCF[Temp->DurM].Surr = ((1 - s) * (Pol->PtrCF.CV[Temp->Dur] - Temp->SBbegin) +
				s * (Pol->PtrCF.CV[Temp->Dur + 1] + Temp->SBend));
			//Other listed benefit
			for (ben = 0; ben < AllHeap[ProdNo].Heap.TotNo.NoBenft; ben++) {
				CalcBen(ProdNo, Pol, ResCF, Temp, ben, Age);
			}
			Temp->DurM++;
		}
		Temp->Dur++;
	}
#pragma endregion

}

__inline void CalcBen(int ProdNo, pols *Pol, resCF *ResCF, temp *Temp, int ben, int Age)
{
	double TempBase, TempCmp;
	int    i;

	if (0 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		TempBase = 0;
		if (Temp->Dur <= Pol->PtrCF.BenDurED[ben] && Temp->Dur >= Pol->PtrCF.BenDurST[ben]) {
			if (1 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].CmpInd) {
				for (i = 0; i < AllHeap[ProdNo].Heap.VAsmp.Benft[ben].NoBase; i++) {
					if (0 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = AllHeap[ProdNo].Heap.VAsmp.Benft[ben].Value / Pol->PtrCF.SATimes;
					}
					else if (1 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = SABASE;
					}
					else if (2 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = Pol->PtrCF.PremAnnl;
					}
					else if (3 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = Pol->PtrCF.CV[Temp->Dur];
					}
					else if (4 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = Pol->PtrCF.PremTerm * Temp->PaidPremTerm;
					}
					else if (5 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempCmp = Pol->PtrCF.PremTerm * Temp->LeftPremTerm;
					}
					if (TempBase < TempCmp) {
						TempBase = TempCmp;
					}
				}
			}
			else if (2 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].CmpInd) {
				for (i = 0; i < AllHeap[ProdNo].Heap.VAsmp.Benft[ben].NoBase; i++) {
					if (0 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += AllHeap[ProdNo].Heap.VAsmp.Benft[ben].Value / Pol->PtrCF.SATimes;
					}
					else if (1 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += SABASE;
					}
					else if (2 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += Pol->PtrCF.PremAnnl;
					}
					else if (3 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += Pol->PtrCF.CV[Temp->Dur];
					}
					else if (4 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += Pol->PtrCF.PremTerm * Temp->PaidPremTerm;
					}
					else if (5 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BasePos[i]) {
						TempBase += Pol->PtrCF.PremTerm * Temp->LeftPremTerm;
					}
				}
			}
		}
		TempBase = TempBase * AllHeap[ProdNo].Heap.VAsmp.Benft[ben].Ratio;
		if (0 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].ImprvMthd) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = TempBase * pow(AllHeap[ProdNo].Heap.VAsmp.Benft[ben].ImprvRT_Y, Temp->Dur);
		}
		else {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = TempBase * pow(AllHeap[ProdNo].Heap.VAsmp.Benft[ben].ImprvRT_M, Temp->DurM);
		}
	}
	if (1 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (Temp->DurM < AllHeap[ProdNo].Heap.PAsmp.WaitPeriod) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm;
		}
		else if (Age < 18) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm * 2;
		}
		else {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = SABASE;
		}
	}
	else if (2 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (Temp->DurM < AllHeap[ProdNo].Heap.PAsmp.WaitPeriod) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm;
		}
		else {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = SABASE;
		}
	}
	else if (3 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (Temp->DurM < AllHeap[ProdNo].Heap.PAsmp.WaitPeriod) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm;
		}
		else {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = 0.2 * SABASE + Temp->LeftPremTerm * Pol->PtrCF.PremTerm;
		}
	}
	else if (4 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (Temp->DurM < AllHeap[ProdNo].Heap.PAsmp.WaitPeriod) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = 0;
		}
		else {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = 0.3 * SABASE;
		}
	}
	else if (5 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm;
	}
	else if (6 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (0 == Temp->DurM % NOMperY && 0 != Temp->DurM && Temp->Dur < Pol->BenEndAge) {
			ResCF->NodeCF[Temp->DurM].Ben[ben] = Temp->PaidPremTerm * Pol->PtrCF.PremTerm * 0.04;
		}
	}
	else if (7 == AllHeap[ProdNo].Heap.VAsmp.Benft[ben].BenID) {
		if (0 == Temp->DurM) {
			ResCF->NodeCF[Pol->DurED - 1].Ben[ben] = SABASE;
		}
	}
}

void ThreadCalc(LPVOID pM)
{
#pragma region Initialization
	int     i, j, sensi, ben, AgentPos, ReCalc = 0, dur, EffY;
	double *LoopRate, StartLx;
	temp    Temp;
	tInfo  *Tinfo  = (tInfo *)pM;
	resCF  *ResCF  = malloc(sizeof(resCF));
	resCF  *CF     = malloc(sizeof(resCF));
	resRT  *ResRT  = malloc(sizeof(resCF));
#pragma endregion

	for (sensi = 0; sensi < AllHeap[Tinfo->ProdNo].Heap.TotNo.NoSensi; sensi++) {
		for (i = Tinfo->StartNo; i <= Tinfo->EndNo; i++) {
			EffY = AllHeap[Tinfo->ProdNo].Heap.Pols[i].EffDate / 10000;
			if (AllHeap[Tinfo->ProdNo].Heap.Pols[i].ReCalcRT) {
				memset(ResRT, 0, sizeof(resRT));
				memset(ResCF, 0, sizeof(resCF));
				Temp.Dur = 0;
				Temp.DurM = 0;
				CalcRate(Tinfo->ProdNo, &AllHeap[Tinfo->ProdNo].Heap.Pols[i], ResRT, &Temp, sensi);
				if (Temp.DurM > TOTLEN) {
					printf("error!\n");
				}
				Temp.Dur = 0;
				Temp.DurM = 0;
				Temp.PaidPremTerm = 0;
				Temp.LeftPremTerm = AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.TotPremTerm;
				Temp.SBbegin = 0;
				Temp.SBend = 0;
				CalcCF(Tinfo->ProdNo, &AllHeap[Tinfo->ProdNo].Heap.Pols[i], ResCF, &Temp);
				if (Temp.DurM > TOTLEN) {
					printf("error!\n");
				}
				ReCalc = 1;
			}
			else if (AllHeap[Tinfo->ProdNo].Heap.Pols[i].ReCalcCF) {
				memset(ResCF, 0, sizeof(resCF));
				Temp.Dur = 0;
				Temp.DurM = 0;
				Temp.PaidPremTerm = 0;
				Temp.LeftPremTerm = AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.TotPremTerm;
				Temp.SBbegin = 0;
				Temp.SBend = 0;
				CalcCF(Tinfo->ProdNo, &AllHeap[Tinfo->ProdNo].Heap.Pols[i], ResCF, &Temp);
				if (Temp.DurM > TOTLEN) {
					printf("error!\n");
				}
				ReCalc = 1;
			}
			if (ReCalc) {
				memset(CF, 0, sizeof(resCF));
				for (j = 0; j < AllHeap[Tinfo->ProdNo].Heap.Pols[i].DurED; j++) {
					CF->NodeCF[j].Prem      = ResCF->NodeCF[j].Prem      * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].Comm      = ResCF->NodeCF[j].Comm      * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].CommOR    = ResCF->NodeCF[j].CommOR    * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].VarExp    = ResCF->NodeCF[j].VarExp    * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].FixExp    = ResCF->NodeCF[j].FixExp    * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].CircFee   = ResCF->NodeCF[j].CircFee   * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].SaftyFund = ResCF->NodeCF[j].SaftyFund * ResRT->NodeRT[j].lx;
					CF->NodeCF[j].Surr      = ResCF->NodeCF[j].Surr      * ResRT->NodeRT[j].lpx;
					LoopRate = &ResRT->NodeRT[j].lx;
					for (ben = 0; ben < AllHeap[Tinfo->ProdNo].Heap.TotNo.NoBenft; ben++) {
						CF->NodeCF[j].Ben[ben] = ResCF->NodeCF[j].Ben[ben] * LoopRate[AllHeap[Tinfo->ProdNo].Heap.VAsmp.Benft[ben].RateID];
					}
				}
				ReCalc = 0;
			}

			StartLx = ResRT->NodeRT[AllHeap[Tinfo->ProdNo].Heap.Pols[i].Dur].lx;
			if (StartLx > 0) {
				AgentPos = AllHeap[Tinfo->ProdNo].Heap.Pols[i].AgentPos + sensi;
				if (AgentPos > AllHeap[Tinfo->ProdNo].TotNoalloc) {
					fprintf_s(fpMain, "Fartal error: Memory overflow!\n");
					return;
				}
				AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].Agent = AllHeap[Tinfo->ProdNo].Heap.Pols[i].Agent;
				AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].Sensi = sensi;
				dur = AllHeap[Tinfo->ProdNo].Heap.Pols[i].Dur;
				for (j = 0; j < AllHeap[Tinfo->ProdNo].Heap.Pols[i].PeriodED; j++) {
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].Prem      += CF->NodeCF[dur].Prem      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].Comm      += CF->NodeCF[dur].Comm      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].CommOR    += CF->NodeCF[dur].CommOR    * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].VarExp    += CF->NodeCF[dur].VarExp    * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].FixExp    += CF->NodeCF[dur].FixExp    * AllHeap[Tinfo->ProdNo].Heap.PAsmp.Infl[j] / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].CircFee   += CF->NodeCF[dur].CircFee   * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].SaftyFund += CF->NodeCF[dur].SaftyFund * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j + 1].Surr  += CF->NodeCF[dur].Surr      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					for (ben = 0; ben < AllHeap[Tinfo->ProdNo].Heap.TotNo.NoBenft; ben++) {
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF.NodeCF[j].Ben[ben] += CF->NodeCF[dur].Ben[ben] * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes / StartLx;
					}
					dur++;
				}
				if (EffY == GlobAsmp.ValY) {
					AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].K_Need = 1;
					for (j = 0; j < AllHeap[Tinfo->ProdNo].Heap.Pols[i].DurED; j++) {
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].Prem      += CF->NodeCF[j].Prem      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].Comm      += CF->NodeCF[j].Comm      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].CommOR    += CF->NodeCF[j].CommOR    * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].VarExp    += CF->NodeCF[j].VarExp    * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].FixExp    += CF->NodeCF[j].FixExp    * AllHeap[Tinfo->ProdNo].Heap.PAsmp.Infl[j];
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].CircFee   += CF->NodeCF[j].CircFee   * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].SaftyFund += CF->NodeCF[j].SaftyFund * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j + 1].Surr  += CF->NodeCF[j].Surr      * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						for (ben = 0; ben < AllHeap[Tinfo->ProdNo].Heap.TotNo.NoBenft; ben++) {
							AllHeap[Tinfo->ProdNo].Heap.AgentRes[AgentPos].CF_K.NodeCF[j].Ben[ben] += CF->NodeCF[dur].Ben[ben] * AllHeap[Tinfo->ProdNo].Heap.Pols[i].PtrCF.SATimes;
						}
					}
				}
			}
		}
	}

	
#pragma region Free
	free(ResCF);
	free(ResRT);
	free(CF);
#pragma endregion
	return;
}