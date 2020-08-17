#include "Head.h"

int LoadGlable(void)
{
	PGresult *res = NULL;
	int       i, j;
	char      sql[SQLLEN];
	double    TempDisc[NOYEAR + 1];

#pragma region Loading global assumption
	GlobAsmp.mfac = (double)1 / (double)NOMperY;
	//============================== Discrate ==============================
	strcpy_s(&sql[0], SQLLEN, "select RatesDisc from DiscRates where ValDate = ");
	_itoa_s(GlobAsmp.ValDate, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
	strcat_s(&sql[0], sizeof(sql), " order by DiscNo desc");

	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpMain)) { return FAIL; };

	GlobAsmp.NoDisc = PQntuples(res);
	//------------------------- Check -------------------------
	if (GlobAsmp.NoDisc > 10) { 
		fprintf_s(fpMain, "Number of discount rate excced maximum of %d", NODISC);
		return FAIL; 
	}
	//---------------------------------------------------------
	for (i = 0; i < GlobAsmp.NoDisc; i++) {
		memset(&TempDisc[0], 0, sizeof(TempDisc));
		ParseArray(&TempDisc[0], NOYEAR + 1, PQgetvalue(res, i, 0), TAILEQPREV);
		GlobAsmp.AccRate[i][0] = 1;
		for (j = 1; j < TOTLEN; j++) {
			GlobAsmp.AccRate[i][j] = GlobAsmp.AccRate[i][j - 1] / pow(1 + TempDisc[((int)j - 1) / 12], GlobAsmp.mfac);
		}
	}

	PQclear(res);
#pragma endregion
	
	fprintf_s(fpMain, "Total of %d discount rates loaded.\n", GlobAsmp.NoDisc);

#pragma region Initialize product list
	//============================== Tot prod ==============================
	strcpy_s(&sql[0], SQLLEN, "select planid from prodasmp");
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpMain)) { return FAIL; };
	GlobAsmp.NoProd = PQntuples(res);
	AllHeap = malloc(sizeof(allHeap) * GlobAsmp.NoProd);
	//------------------------- Check -------------------------
	if (!AllHeap) { 
		fprintf_s(fpMain, "Initial malloc for AllHeap fail!\n");
		return FAIL; 
	}
	//---------------------------------------------------------
	memset(AllHeap, 0, sizeof(allHeap) * GlobAsmp.NoProd);
	for (i = 0; i < GlobAsmp.NoProd; i++) {
		AllHeap[i].PlanID = atoi(PQgetvalue(res, i, 0));
	}
	PQclear(res);
#pragma endregion

	fprintf_s(fpMain, "Total of %d products will be valued.\n", GlobAsmp.NoProd);

	return SUCCESS;
}

void ThreadLoadProd(LPVOID pM)
{
	int       i;
	PGresult *res = NULL;
	char      sql[SQLLEN];
	clock_t   begin, end;

	fprintf_s(fpAsmp, "Total of %d products' assumption will be loaded.\n", GlobAsmp.NoProd);

	for (i = 0; i < GlobAsmp.NoProd; i++) {
		begin = clock();
		fprintf_s(fpAsmp, "====================================================================\n");
		fprintf_s(fpAsmp, "Loading products assumption of %d...\n", AllHeap[i].PlanID);
		
		LoadProd(i);
		LoadPolicys(i);
		
#pragma region Malloc results heap
		fprintf_s(fpAsmp, "Allocating heap memory for product %d...\n", AllHeap[i].PlanID);
		//============================== Data Query ==============================
		strcpy_s(&sql[0], SQLLEN, "select agent from actmop where planid = ");
		_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
		strcat_s(&sql[0], SQLLEN, " group by agent");
		if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
			GlobAsmp.LoadAllHeapFail = 1;
			return;
		}
		AllHeap[i].Heap.NoAgn = PQntuples(res);
		//============================== Data Apply ==============================
		AllHeap[i].TotNoalloc = (AllHeap[i].Heap.NoAgn + NOTHRED) * AllHeap[i].Heap.TotNo.NoSensi;
		while (!(AllHeap[i].Heap.AgentRes = malloc(sizeof(agtRes) * AllHeap[i].TotNoalloc))) {
			AllHeap[i].Heap.AgentRes = NULL;
		};
		memset(AllHeap[i].Heap.AgentRes,0, sizeof(agtRes) * AllHeap[i].TotNoalloc);
#pragma endregion

		end = clock();
		fprintf_s(fpAsmp, "Total %d rows fetched\n", AllHeap[i].Heap.NoPol);
		fprintf_s(fpAsmp, "Total %d Agents loaded\n", AllHeap[i].Heap.NoAgn);
		fprintf_s(fpAsmp, "Total cost: %d millisecond\n", end - begin);
		AllHeap[i].Loaded = 1;
		fprintf_s(fpAsmp, "--------------------------------------------------------------------\n");
	}

	return;
}

void LoadProd(int i)
{
	int       j, k, l, norx = 0, ratst, rated, sex, age;
	char      sql[SQLLEN], *TempStrPtr, *p, *nextp;
	double    TempDouble[NOYEAR], adjfc, imprv, infl;
	PGresult *res = NULL, *res2 = NULL;

#pragma region Loading pref-assumption
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select waitpd,circfee,saftyfd,inflt,maxinf,tpd from prodasmp where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//============================= Data Loading =============================
	AllHeap[i].Heap.PAsmp.WaitPeriod = atoi(PQgetvalue(res, 0, 0));
	_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.PAsmp.CIRCFee, PQgetvalue(res, 0, 1));
	_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.PAsmp.SaftyFund, PQgetvalue(res, 0, 2));
	_atodbl((_CRT_DOUBLE*)&infl, PQgetvalue(res, 0, 3));
	_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.PAsmp.MaxInf, PQgetvalue(res, 0, 4));
	_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.PAsmp.TPD, PQgetvalue(res, 0, 5));
	AllHeap[i].Heap.PAsmp.Infl[0] = 1;
	infl = pow(1 + infl, GlobAsmp.mfac);
	for (j = 1; j < TOTLEN; j++) {
		AllHeap[i].Heap.PAsmp.Infl[j] = min(AllHeap[i].Heap.PAsmp.Infl[j - 1] * infl, AllHeap[i].Heap.PAsmp.MaxInf);
	}
#pragma endregion

#pragma region Sensitivity
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select qx,ix,kx,rx,lpx from prodasmp, unnest(sensi) where planid=");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.TotNo.NoSensi = PQntuples(res);
	//------------------------- Check -------------------------
	AllHeap[i].Heap.VAsmp.Sensi = malloc(sizeof(sensi) * AllHeap[i].Heap.TotNo.NoSensi);
	if (NULL == AllHeap[i].Heap.VAsmp.Sensi) {
		fprintf_s(fpAsmp, "Malloc sensi failed on product %d\n", AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	memset(AllHeap[i].Heap.VAsmp.Sensi, 0, sizeof(sensi) * AllHeap[i].Heap.TotNo.NoSensi);
	//---------------------------------------------------------
	//============================= Data Loading =============================
	for (j = 0; j < AllHeap[i].Heap.TotNo.NoSensi; j++) {
		ParseArray(&TempDouble[0],  NOYEAR, PQgetvalue(res, j, 0), TAILEQPREV);
		for (k = 0; k < NOYEAR; k++) {
			AllHeap[i].Heap.VAsmp.Sensi[j].NodeSN[k].qx = TempDouble[k];
		}
		ParseArray(&TempDouble[0],  NOYEAR, PQgetvalue(res, j, 1), TAILEQPREV);
		for (k = 0; k < NOYEAR; k++) {
			AllHeap[i].Heap.VAsmp.Sensi[j].NodeSN[k].ix = TempDouble[k];
		}
		ParseArray(&TempDouble[0],  NOYEAR, PQgetvalue(res, j, 2), TAILEQPREV);
		for (k = 0; k < NOYEAR; k++) {
			AllHeap[i].Heap.VAsmp.Sensi[j].NodeSN[k].kx = TempDouble[k];
		}
		ParseArray(&TempDouble[0], NOYEAR, PQgetvalue(res, j, 4), TAILEQPREV);
		for (k = 0; k < NOYEAR; k++) {
			AllHeap[i].Heap.VAsmp.Sensi[j].NodeSN[k].lpx = TempDouble[k];
		}
		//rx
		TempStrPtr = malloc(strlen(PQgetvalue(res, j, 3)) + 1);
		//------------------------- Check -------------------------
		if (!TempStrPtr) {
			fprintf_s(fpAsmp, "Malloc TempStrPtr fail!\n");
			GlobAsmp.LoadAllHeapFail = 1;
			return;
		}
		//---------------------------------------------------------
		strcpy_s(TempStrPtr, strlen(PQgetvalue(res, j, 3)) + 1, PQgetvalue(res, j, 3));
		p = strtok_s(TempStrPtr, "},{", &nextp);
		if (p != NULL) {
			norx = 0;
			while (p) {
				ParseArray(&TempDouble[0], NOYEAR, p, TAILEQPREV);
				for (k = 0; k < NOYEAR; k++) {
					AllHeap[i].Heap.VAsmp.Sensi[j].NodeSN[k].rx[norx] = TempDouble[k];
				}
				norx++;
				//------------------------- Check -------------------------
				if (norx > NORIDER) {
					fprintf_s(fpAsmp, "Loaded number of rider is %d, exceed max number of rider of %d!\n", norx, NORIDER);
					GlobAsmp.LoadAllHeapFail = 1;
					return;
				}
				//---------------------------------------------------------
				p = strtok_s(NULL, "},{", &nextp);
			}
		}
		free(TempStrPtr);
	}
	AllHeap[i].Heap.TotNo.NoRx = norx;
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion

#pragma region Rates
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select typert,agest,ageed,imprv,adjfc,selfc,ratem,ratef from prodasmp, unnest(rates) where planid=");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//============================= Data Loading =============================
	norx = 0;
	for (j = 0; j < PQntuples(res); j++) {
		if (!strcmp(PQgetvalue(res, j, 0), "qx")) {
			ratst = atoi(PQgetvalue(res, j, 1));
			rated = atoi(PQgetvalue(res, j, 2));
			_atodbl((_CRT_DOUBLE*)&imprv, PQgetvalue(res, j, 3));
			_atodbl((_CRT_DOUBLE*)&adjfc, PQgetvalue(res, j, 4));
			ParseArray(&TempDouble[0], NOYEAR, PQgetvalue(res, j, 5), TAILEQPREV);
			//Adjust factor, Improve factor, selection factor, apply on sensitivity
			for (k = 0; k < NOYEAR; k++) {
				for (l = 0; l < AllHeap[i].Heap.TotNo.NoSensi; l++) {
					AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].qx = AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].qx * adjfc * TempDouble[k] * pow((1 + imprv), (double)k);
				}
			}
			//Rates
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 6));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[0].qx[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 7));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[1].qx[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
		}
		else if (!strcmp(PQgetvalue(res, j, 0), "ix")) {
			ratst = atoi(PQgetvalue(res, j, 1));
			rated = atoi(PQgetvalue(res, j, 2));
			_atodbl((_CRT_DOUBLE*)&imprv, PQgetvalue(res, j, 3));
			_atodbl((_CRT_DOUBLE*)&adjfc, PQgetvalue(res, j, 4));
			ParseArray(&TempDouble[0], NOYEAR, PQgetvalue(res, j, 5), TAILEQPREV);
			//Adjust factor, Improve factor, selection factor, apply on sensitivity
			for (k = 0; k < NOYEAR; k++) {
				for (l = 0; l < AllHeap[i].Heap.TotNo.NoSensi; l++) {
					AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].ix = AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].ix * adjfc * TempDouble[k] * pow((1 + imprv), (double)k);
				}
			}
			//Rates
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 6));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[0].ix[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 7));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[1].ix[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
		}
		else if (!strcmp(PQgetvalue(res, j, 0), "kx")) {
			ratst = atoi(PQgetvalue(res, j, 1));
			rated = atoi(PQgetvalue(res, j, 2));
			_atodbl((_CRT_DOUBLE*)&imprv, PQgetvalue(res, j, 3));
			_atodbl((_CRT_DOUBLE*)&adjfc, PQgetvalue(res, j, 4));
			ParseArray(&TempDouble[0], NOYEAR, PQgetvalue(res, j, 5), TAILEQPREV);
			//Adjust factor, Improve factor, selection factor, apply on sensitivity
			for (k = 0; k < NOYEAR; k++) {
				for (l = 0; l < AllHeap[i].Heap.TotNo.NoSensi; l++) {
					AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].kx = AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].kx * adjfc * TempDouble[k] * pow((1 + imprv), (double)k);
				}
			}
			//Rates
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 6));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[0].kx[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 7));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[1].kx[ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
		}
		else {
			ratst = atoi(PQgetvalue(res, j, 1));
			rated = atoi(PQgetvalue(res, j, 2));
			_atodbl((_CRT_DOUBLE*)&imprv, PQgetvalue(res, j, 3));
			_atodbl((_CRT_DOUBLE*)&adjfc, PQgetvalue(res, j, 4));
			ParseArray(&TempDouble[0], NOYEAR, PQgetvalue(res, j, 5), TAILEQPREV);
			//Adjust factor, Improve factor, selection factor, apply on sensitivity
			for (k = 0; k < NOYEAR; k++) {
				for (l = 0; l < AllHeap[i].Heap.TotNo.NoSensi; l++) {
					AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].rx[norx] = AllHeap[i].Heap.VAsmp.Sensi[l].NodeSN[k].rx[norx] * adjfc * TempDouble[k] * pow((1 + imprv), (double)k);
				}
			}
			//Rates
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 6));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[0].rx[norx][ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
			strcpy_s(&sql[0], SQLLEN, "select rates from decrates where descrpt = '");
			strcat_s(&sql[0], SQLLEN, PQgetvalue(res, j, 7));
			strcat_s(&sql[0], SQLLEN, "'");
			if (DBExec(GlobAsmp.conn, &res2, &sql[0], fpAsmp)) { GlobAsmp.LoadAllHeapFail = 1; return; }
			ParseArray(&AllHeap[i].Heap.VAsmp.Rates[1].rx[norx][ratst], rated - ratst, PQgetvalue(res2, 0, 0), TAILNA);
			PQclear(res2);
			norx++;
		}
	}
	//------------------------- Check -------------------------
	if (norx != AllHeap[i].Heap.TotNo.NoRx) {
		fprintf_s(fpAsmp, "Number of Rider rates is %d, does not match loaded sensitivity assumption as of!\n", norx);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//---------------------------------------------------------
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion

#pragma region NP related assumption
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select channel,np,ratepre,ratepos,skewpre,skewpos,adjye,comm,commor,fyape,fixexp,varexp from prodasmp, unnest(asmpnp) where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.TotNo.NoVarNP = PQntuples(res);
	//------------------------- Check -------------------------
	AllHeap[i].Heap.VAsmp.VarNP = malloc(sizeof(varnp) * AllHeap[i].Heap.TotNo.NoVarNP);
	if (NULL == AllHeap[i].Heap.VAsmp.VarNP) {
		fprintf_s(fpAsmp, "Malloc varnp failed on product %d\n", AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//---------------------------------------------------------
	memset(AllHeap[i].Heap.VAsmp.VarNP, 0, sizeof(varnp) * AllHeap[i].Heap.TotNo.NoVarNP);
	//============================= Data Loading =============================
	for (j = 0; j < PQntuples(res); j++) {
		AllHeap[i].Heap.VAsmp.VarNP[j].Chnl = atoi(PQgetvalue(res, j, 0));
		strcpy_s(&AllHeap[i].Heap.VAsmp.VarNP[j].NP[0], TERMLEN / 2, PQgetvalue(res, j, 1));
		//Lapse
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].lpxPre[0],  NOYEAR,  PQgetvalue(res, j, 2),  TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].lpxPos[0],  NOYEAR,  PQgetvalue(res, j, 3),  TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].SkewPre[0], NOMperY, PQgetvalue(res, j, 4),  TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].SkewPos[0], NOMperY, PQgetvalue(res, j, 5),  TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].AdjYE[0],   NOYEAR,  PQgetvalue(res, j, 6),  TAILEQPREV);
		//Commission
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].Comm[0],    NOYEAR,  PQgetvalue(res, j, 7),  TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].CommOR[0],  NOYEAR,  PQgetvalue(res, j, 8),  TAILEQPREV);
		//Expense
		AllHeap[i].Heap.VAsmp.VarNP[j].FYAPE = atoi(PQgetvalue(res, j, 9));
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].FixExp[0],  NOYEAR,  PQgetvalue(res, j, 10), TAILEQPREV);
		ParseArray(&AllHeap[i].Heap.VAsmp.VarNP[j].VarExp[0],  NOYEAR,  PQgetvalue(res, j, 11), TAILEQPREV);
		for (k = 0; k < NOYEAR; k++) {
			AllHeap[i].Heap.VAsmp.VarNP[j].FixExp[k] = AllHeap[i].Heap.VAsmp.VarNP[j].FixExp[k] * GlobAsmp.mfac;
		}
	}
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion

#pragma region Benefit
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select benid,rateid,baseind,cmpind,prdst,prded,ratio,imprvy,val,ImpMthd from prodasmp,unnest(benft) where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.TotNo.NoBenft = PQntuples(res);
	//------------------------- Check -------------------------
	if (AllHeap[i].Heap.TotNo.NoBenft > NOBEN) {
		fprintf_s(fpAsmp, "Number of benefit scaned is %d, exceed the maximun of %d for product %d\n", AllHeap[i].Heap.TotNo.NoBenft, NOBEN, AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//---------------------------------------------------------
	//============================= Data Loading =============================
	for (j = 0; j < AllHeap[i].Heap.TotNo.NoBenft; j++) {
		AllHeap[i].Heap.VAsmp.Benft[j].BenID     = atoi(PQgetvalue(res, j, 0));
		AllHeap[i].Heap.VAsmp.Benft[j].RateID    = atoi(PQgetvalue(res, j, 1)); 
		AllHeap[i].Heap.VAsmp.Benft[j].ImprvMthd = atoi(PQgetvalue(res, j, 9));
		AllHeap[i].Heap.VAsmp.Benft[j].CmpInd    = atoi(PQgetvalue(res, j, 3));
		strcpy_s(&AllHeap[i].Heap.VAsmp.Benft[j].Start[0], TERMLEN / 2,  PQgetvalue(res, j, 4));
		strcpy_s(&AllHeap[i].Heap.VAsmp.Benft[j].End[0],   TERMLEN / 2,  PQgetvalue(res, j, 5));
		_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.VAsmp.Benft[j].Ratio,     PQgetvalue(res, j, 6));
		_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.VAsmp.Benft[j].ImprvRT_Y, PQgetvalue(res, j, 7));
		_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.VAsmp.Benft[j].Value,     PQgetvalue(res, j, 8));
		AllHeap[i].Heap.VAsmp.Benft[j].ImprvRT_Y = 1 + AllHeap[i].Heap.VAsmp.Benft[j].ImprvRT_Y;
		AllHeap[i].Heap.VAsmp.Benft[j].ImprvRT_M = pow(AllHeap[i].Heap.VAsmp.Benft[j].ImprvRT_Y, GlobAsmp.mfac);
		AllHeap[i].Heap.VAsmp.Benft[j].NoBase = ParseArrayInt(&AllHeap[i].Heap.VAsmp.Benft[j].BasePos[0], NOBASE, PQgetvalue(res, j, 2), TAILNA);
	}
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion

#pragma region K_factor
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select kyear,kvalue from prodasmp,unnest(k_fact) where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.TotNo.NoKfact = PQntuples(res);
	//------------------------- Check -------------------------
	AllHeap[i].Heap.VAsmp.KFact = malloc(sizeof(kfac) * AllHeap[i].Heap.TotNo.NoKfact);
	if (NULL == AllHeap[i].Heap.VAsmp.KFact) {
		fprintf_s(fpAsmp, "Malloc kfac failed on product %d\n", AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//---------------------------------------------------------
	memset(AllHeap[i].Heap.VAsmp.KFact, 0, sizeof(kfac) * AllHeap[i].Heap.TotNo.NoKfact);
	//============================= Data Loading =============================
	for (j = 0; j < AllHeap[i].Heap.TotNo.NoKfact; j++) {
		AllHeap[i].Heap.VAsmp.KFact[j].Year = atoi(PQgetvalue(res, j, 0));
		_atodbl((_CRT_DOUBLE*)&AllHeap[i].Heap.VAsmp.KFact[j].K_Fac, PQgetvalue(res, j, 1));
	}
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion

#pragma region GCV
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "select np from factor where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	strcat_s(&sql[0], SQLLEN, " group by np");
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.TotNo.NoGCV = PQntuples(res);
	//------------------------- Check -------------------------
	AllHeap[i].Heap.VAsmp.GCV = malloc(sizeof(gcv) * AllHeap[i].Heap.TotNo.NoGCV);
	if (NULL == AllHeap[i].Heap.VAsmp.GCV) {
		fprintf_s(fpAsmp, "Malloc GCV failed on product %d\n", AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//---------------------------------------------------------
	memset(AllHeap[i].Heap.VAsmp.GCV, 0, sizeof(gcv) * AllHeap[i].Heap.TotNo.NoGCV);
	PQclear(res);
	strcpy_s(&sql[0], SQLLEN, "select * from factor where planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], SQLLEN - strlen(sql), 10);
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	//============================= Data Loading =============================
	for (j = 0; j < PQntuples(res); j++) {
		sex = atoi(PQgetvalue(res, j, 2));
		age = atoi(PQgetvalue(res, j, 4));
		for (k = 0; k < AllHeap[i].Heap.TotNo.NoGCV; k++) {
			if (0 == AllHeap[i].Heap.VAsmp.GCV[k].NP[0]) {
				strcpy_s(&AllHeap[i].Heap.VAsmp.GCV[k].NP[0], TERMLEN / 2, PQgetvalue(res, j, 3));
				break;
			}
			else if (!strcmp(&AllHeap[i].Heap.VAsmp.GCV[k].NP[0], PQgetvalue(res, j, 3))) {
				break;
			}
		}
		if (!strcmp(PQgetvalue(res, j, 1), "CV")) {
			ParseArray(&AllHeap[i].Heap.VAsmp.GCV[k].CV[sex][age][1], NOYEAR, PQgetvalue(res, j, 5), TAILNA);
		}
		else if (!strcmp(PQgetvalue(res, j, 1), "CVNP")) {
			ParseArray(&AllHeap[i].Heap.VAsmp.GCV[k].CVNP[sex][age][1], NOYEAR, PQgetvalue(res, j, 5), TAILNA);
		}
		else {
			fprintf_s(fpAsmp, "Unknown CV type for product %d", AllHeap[i].PlanID);
			return;
		}
	}
	//============================= Clear reslts =============================
	PQclear(res);
#pragma endregion
	
}

void LoadPolicys(int i)
{
	int       j, PolPerTh, ThreadNo;
	char      sql[SQLLEN];
	double    Prem, SA;
	PGresult *res = NULL;

	fprintf_s(fpAsmp, "Loading products policy of %d...\n", AllHeap[i].PlanID);

#pragma region Excecute query
	strcpy_s(&sql[0], SQLLEN, "select agent,chnl,sex,issage,paymode,payingyr,benefitpr,effdate,prem_annual,sa,polno,1,prem_annual/sa from actmop where status in (101,102) and effdate<=");
	_itoa_s(GlobAsmp.ValDate, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
	strcat_s(&sql[0], SQLLEN, " and planid = ");
	_itoa_s(AllHeap[i].PlanID, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
	strcat_s(&sql[0], SQLLEN, " order by agent,chnl,sex,issage,payingyr,prem_annual/sa,effdate");
	if (DBExec(GlobAsmp.conn, &res, &sql[0], fpAsmp)) {
		GlobAsmp.LoadAllHeapFail = 1;
		return;
	}
	AllHeap[i].Heap.NoPol = PQntuples(res);
	AllHeap[i].Heap.Pols = malloc(sizeof(pols) * AllHeap[i].Heap.NoPol);
	if (!AllHeap[i].Heap.Pols) {
		fprintf_s(fpAsmp, "Malloc policy failed on product %d\n", AllHeap[i].PlanID);
		GlobAsmp.LoadAllHeapFail = 1;
	}
	memset(AllHeap[i].Heap.Pols, 0, sizeof(pols) * AllHeap[i].Heap.NoPol);
#pragma endregion

#pragma region Filling pol structure
	ThreadNo = 0;
	PolPerTh = AllHeap[i].Heap.NoPol / NOTHRED;
	for (j = 0; j < AllHeap[i].Heap.NoPol; j++) {
		//================== Pre-fill basic info ==================
		AllHeap[i].Heap.Pols[j].Count   = atoi(PQgetvalue(res, j, 11));
		AllHeap[i].Heap.Pols[j].Agent   = atoi(PQgetvalue(res, j, 0));
		AllHeap[i].Heap.Pols[j].EffDate = atoi(PQgetvalue(res, j, 7));
		_atodbl((_CRT_DOUBLE*)&Prem, PQgetvalue(res, j, 8));
		_atodbl((_CRT_DOUBLE*)&SA, PQgetvalue(res, j, 9));
		//---------------------------------------------------------
		//================== Pre-fill basic info ==================
		if (j > 0) {
			if (AllHeap[i].Heap.Pols[j].Agent != AllHeap[i].Heap.Pols[j - 1].Agent) {
				AllHeap[i].MaxAgntPos += AllHeap[i].Heap.TotNo.NoSensi;
			}
			else if ((PolPerTh > 0 && 0 == j % PolPerTh) || PolPerTh == 0) {
				ThreadNo++;
				if (ThreadNo < NOTHRED) {
					AllHeap[i].MaxAgntPos += AllHeap[i].Heap.TotNo.NoSensi;
					AllHeap[i].TInfo[ThreadNo - 1].EndNo = j - 1;
					AllHeap[i].TInfo[ThreadNo].StartNo = j;
				}
			}
		}
		AllHeap[i].Heap.Pols[j].AgentPos = AllHeap[i].MaxAgntPos;
		//---------------------------------------------------------
		//=============== Filling in pols structure ===============
		FillPolicy(i, &AllHeap[i].Heap.Pols[j],
			PQgetvalue(res, j, 5),
			PQgetvalue(res, j, 6),
			PQgetvalue(res, j, 10),
			atoi(PQgetvalue(res, j, 1)),
			atoi(PQgetvalue(res, j, 2)),
			atoi(PQgetvalue(res, j, 3)),
			atoi(PQgetvalue(res, j, 4)),
			Prem,
			SA
		);
		//---------------------------------------------------------
		//============ Analyze need for re-calculation ============
		if (j == AllHeap[i].TInfo[ThreadNo].StartNo) {
			AllHeap[i].Heap.Pols[j].ReCalcCF = 1;
			AllHeap[i].Heap.Pols[j].ReCalcRT = 1;
		}
		else {
			if (strcmp(PQgetvalue(res, j, 1), PQgetvalue(res, j - 1, 1)) ||
				strcmp(PQgetvalue(res, j, 2), PQgetvalue(res, j - 1, 2)) ||
				strcmp(PQgetvalue(res, j, 3), PQgetvalue(res, j - 1, 3)) ||
				strcmp(PQgetvalue(res, j, 5), PQgetvalue(res, j - 1, 5))) {
				AllHeap[i].Heap.Pols[j].ReCalcRT = 1;
				AllHeap[i].Heap.Pols[j].ReCalcCF = 1;
			}
			if (strcmp(PQgetvalue(res, j, 4), PQgetvalue(res, j - 1, 4)) ||
				strcmp(PQgetvalue(res, j, 12), PQgetvalue(res, j - 1, 12))) {
				AllHeap[i].Heap.Pols[j].ReCalcCF = 1;
			}
		}

		//---------------------------------------------------------
	}
	AllHeap[i].TInfo[NOTHRED - 1].EndNo = AllHeap[i].Heap.NoPol - 1;
#pragma endregion

	/*
	FILE *fpcheck;
	char  TempChar[SQLLEN];
	sprintf_s(&TempChar[0], SQLLEN, "PolicyCheck_%d.csv", AllHeap[i].PlanID);
	fopen_s(&fpcheck, &TempChar[0], "w");
	fprintf_s(fpcheck, "ThreadNo,PolStart,PolEnd\n");
	for (j = 0; j < NOTHRED; j++) {
		fprintf_s(fpcheck, "%d,%d,%d\n", j, AllHeap[i].TInfo[j].StartNo, AllHeap[i].TInfo[j].EndNo);
	}
	fprintf_s(fpcheck, "Agent,chnl,sex,issage,NP,paymode,Prem/sa,EffDate,AgentPos,Dur,PayEndAge,BenEndAge,PeriodED,DurED,ReCalcRT,ReCalcCF,TotPremTer,SATimes,PremTerm\n");
	for (j = 0; j < AllHeap[i].Heap.NoPol; j++) {
		fprintf_s(fpcheck, "%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f\n",
			PQgetvalue(res, j, 0),
			PQgetvalue(res, j, 1),
			PQgetvalue(res, j, 2),
			PQgetvalue(res, j, 3),
			PQgetvalue(res, j, 5),
			PQgetvalue(res, j, 4),
			PQgetvalue(res, j, 12),
			PQgetvalue(res, j, 7),
			AllHeap[i].Heap.Pols[j].AgentPos,
			AllHeap[i].Heap.Pols[j].Dur,
			AllHeap[i].Heap.Pols[j].PayEndAge,
			AllHeap[i].Heap.Pols[j].BenEndAge,
			AllHeap[i].Heap.Pols[j].PeriodED,
			AllHeap[i].Heap.Pols[j].DurED,
			AllHeap[i].Heap.Pols[j].ReCalcRT,
			AllHeap[i].Heap.Pols[j].ReCalcCF,
			AllHeap[i].Heap.Pols[j].PtrCF.TotPremTerm,
			AllHeap[i].Heap.Pols[j].PtrCF.SATimes,
			AllHeap[i].Heap.Pols[j].PtrCF.PremTerm
		);
	}
	fclose(fpcheck);
	*/

#pragma region Clear query results
	PQclear(res);
#pragma endregion

}

inline void FillPolicy(int i, pols *Pol, char *NP, char *NB, char *PolNo, int Chnl, int Sex, int issAge, int PayMode, double Prem, double SA)
{
	int j;

#pragma region NB & NP
	if (64 == NP[0]) {
		Pol->PayEndAge = atoi(&NP[1]) - 1;
	}
	else {
		Pol->PayEndAge = issAge + atoi(&NP[0]) - 1;
	}
	if (64 == NB[0]) {
		Pol->BenEndAge = atoi(&NB[1]);
	}
	else {
		Pol->BenEndAge = issAge + atoi(&NB[0]);
	}
#pragma endregion

#pragma region Logical check
	if (Pol->BenEndAge < Pol->PayEndAge) {
		Pol->Error = 1;
		fprintf_s(fpAsmp, "Fartal logcal error: Pay period > benefti period for PolNo: %s, policy will not be valued\n", PolNo);
	}
	else if (Pol->BenEndAge > NOYEAR) {
		fprintf_s(fpAsmp, "Logcal error: benefti period > %d for PolNo: %s, adjust to %d\n", NOYEAR, PolNo, NOYEAR);
		Pol->BenEndAge = NOYEAR;
	}
	else if (Sex > 1 || Sex < 0) {
		Pol->Error = 1;
		fprintf_s(fpAsmp, "Fartal logcal error: Sex code violation (value as %d) for PolNo: %s, policy will not be valued\n", Sex, PolNo);
	}
#pragma endregion

#pragma region Match NP variant assumption
	for (j = 0; j < AllHeap[i].Heap.TotNo.NoVarNP; j++) {
		if (Chnl == AllHeap[i].Heap.VAsmp.VarNP[j].Chnl && !strcmp(&NP[0], &AllHeap[i].Heap.VAsmp.VarNP[j].NP[0])) {
			//Lapse
			Pol->PtrRate.AdjYE   = &AllHeap[i].Heap.VAsmp.VarNP[j].AdjYE[0];
			Pol->PtrRate.lpxPre  = &AllHeap[i].Heap.VAsmp.VarNP[j].lpxPre[0];
			Pol->PtrRate.lpxPos  = &AllHeap[i].Heap.VAsmp.VarNP[j].lpxPos[0];
			Pol->PtrRate.SkewPre = &AllHeap[i].Heap.VAsmp.VarNP[j].SkewPre[0];
			Pol->PtrRate.SkewPos = &AllHeap[i].Heap.VAsmp.VarNP[j].SkewPos[0];
			//Commission
			Pol->PtrCF.Comm      = &AllHeap[i].Heap.VAsmp.VarNP[j].Comm[0];
			Pol->PtrCF.CommOR    = &AllHeap[i].Heap.VAsmp.VarNP[j].CommOR[0];
			//Expense
			Pol->PtrCF.FYAPE     =  AllHeap[i].Heap.VAsmp.VarNP[j].FYAPE;
			Pol->PtrCF.FixExp    = &AllHeap[i].Heap.VAsmp.VarNP[j].FixExp[0];
			Pol->PtrCF.VarExp    = &AllHeap[i].Heap.VAsmp.VarNP[j].VarExp[0];
			break;
		}
	}
	//Check
	if (j == AllHeap[i].Heap.TotNo.NoVarNP) {
		Pol->Error = 1;
		fprintf_s(fpAsmp, "Fartal error: Can not match assumption for Channal %d and NP %s for PolNo: %s, policy will not be valued\n", Chnl, NP, PolNo);
	}
#pragma endregion

#pragma region Match GCV
	for (j = 0; j < AllHeap[i].Heap.TotNo.NoGCV; j++) {
		if (!strcmp(&NP[0], &AllHeap[i].Heap.VAsmp.GCV[j].NP[0])) {
			Pol->PtrCF.CV   = &AllHeap[i].Heap.VAsmp.GCV[j].CV[Sex][issAge][0];
			Pol->PtrCF.CVNP = &AllHeap[i].Heap.VAsmp.GCV[j].CVNP[Sex][issAge][0];
			break;
		}
	}
	if (j == AllHeap[i].Heap.TotNo.NoGCV) {
		Pol->Error = 1;
		fprintf_s(fpAsmp, "Fartal error: Can not match CCV for NP %s for PolNo: %s, policy will not be valued\n", NP, PolNo);
	}
#pragma endregion

	Pol->issAge            = issAge;
	Pol->Dur               = (GlobAsmp.ValY - Pol->EffDate / 10000) * 12 + GlobAsmp.ValM - Pol->EffDate % 10000 / 100;
	Pol->DurED             = min(Pol->BenEndAge * NOMperY + 1 - issAge * NOMperY, NOYEAR * NOMperY);
	Pol->PeriodED          = Pol->DurED - Pol->Dur;
	Pol->PtrRate.Rates     = &AllHeap[i].Heap.VAsmp.Rates[Sex];
	Pol->PtrCF.SATimes     = SA / SABASE;
	Pol->PtrCF.PremAnnl    = Prem / Pol->PtrCF.SATimes;
	Pol->PtrCF.PayMode     = PayMode;
	Pol->PtrCF.PayTerm     = NOMperY / PayMode;
	Pol->PtrCF.PremTerm    = Pol->PtrCF.PremAnnl / (double)PayMode;
	Pol->PtrCF.TotPremTerm = (Pol->PayEndAge - issAge + 1) * PayMode;

	for (j = 0; j < AllHeap[i].Heap.TotNo.NoBenft; j++) {
		if (64 == AllHeap[i].Heap.VAsmp.Benft[j].Start[0]) {
			Pol->PtrCF.BenDurST[j] = atoi(&AllHeap[i].Heap.VAsmp.Benft[j].Start[1]) - Pol->issAge;
		}
		else {
			Pol->PtrCF.BenDurST[j] = atoi(&AllHeap[i].Heap.VAsmp.Benft[j].Start[0]);
		}
		if (64 == AllHeap[i].Heap.VAsmp.Benft[j].End[0]) {
			Pol->PtrCF.BenDurED[j] = atoi(&AllHeap[i].Heap.VAsmp.Benft[j].End[1]) - Pol->issAge;
		}
		else {
			Pol->PtrCF.BenDurED[j] = atoi(&AllHeap[i].Heap.VAsmp.Benft[j].End[0]);
		}
	}
}

void FreeProd(int i)
{
	free(AllHeap[i].Heap.VAsmp.Sensi);
	free(AllHeap[i].Heap.VAsmp.VarNP);
	free(AllHeap[i].Heap.VAsmp.KFact);
	free(AllHeap[i].Heap.VAsmp.GCV);
}