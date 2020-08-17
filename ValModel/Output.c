#include "Head.h"

void ThreadOutP(LPVOID pM)
{
	char       *buffer = malloc(BUFFLEN), sql[SQLLEN];
	int         Status, ProdNo = (int)pM, i, j, ben;
	PGconn     *conn = NULL;
	PGresult   *res;
	clock_t     begin, end;

	fprintf_s(fpOutP, "PlanID %d Start inseting...\n", AllHeap[ProdNo].PlanID);
	begin = clock();
	DBConn(&conn, CONNSTR, fpOutP);

#pragma region General cashflow
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "COPY AgentCF(PlanID,Agent,Senario,Prem,Comm,CommOR,CIRCFee,SaftyFund,FixExp,VarExp,Surr");
	for (ben = 1; ben <= AllHeap[ProdNo].Heap.TotNo.NoBenft; ben++) {
		strcat_s(&sql[0], SQLLEN, ",Benefit_");
		_itoa_s(ben, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
	}
	strcat_s(&sql[0], SQLLEN, ") FROM STDIN;");
	res = PQexec(conn, &sql[0]);
	if (PQresultStatus(res) != PGRES_COPY_IN) {
		fprintf_s(fpOutP, "Not in COPY_IN mode\n");
	}
	PQclear(res);
	//============================== Data Apply ==============================
	for (i = 0; i < AllHeap[ProdNo].MaxAgntPos + AllHeap[ProdNo].Heap.TotNo.NoSensi; i++) {
		sprintf_s(buffer, BUFFLEN, "%d\t%d\t%d\t", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent, AllHeap[ProdNo].Heap.AgentRes[i].Sensi);
		Status = PQputCopyData(conn, buffer, (int)strlen(buffer));
		if (1 != Status) {
			fprintf_s(fpOutP, "PlanID %d inserting error for agent %d\n", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent);
		}
		for (j = 0; j < 8 + AllHeap[ProdNo].Heap.TotNo.NoBenft; j++) {
			if (j == 7 + AllHeap[ProdNo].Heap.TotNo.NoBenft) {
				AsmbCF(buffer, BUFFLEN, ProdNo, i, j, 1);
			}
			else {
				AsmbCF(buffer, BUFFLEN, ProdNo, i, j, 0);
			}
			Status = PQputCopyData(conn, buffer, (int)strlen(buffer));
			if (1 != Status) {
				fprintf_s(fpOutP, "PlanID %d inserting error for agent %d\n", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent);
			}
		}
	}
	Status = PQputCopyEnd(conn, NULL);
#pragma endregion

#pragma region K cashflow
	//============================== Data Query ==============================
	strcpy_s(&sql[0], SQLLEN, "COPY AgentCF_K(PlanID,Agent,Senario,Prem,Comm,CommOR,CIRCFee,SaftyFund,FixExp,VarExp,Surr");
	for (ben = 1; ben <= AllHeap[ProdNo].Heap.TotNo.NoBenft; ben++) {
		strcat_s(&sql[0], SQLLEN, ",Benefit_");
		_itoa_s(ben, &sql[strlen(sql)], sizeof(sql) - strlen(sql), 10);
	}
	strcat_s(&sql[0], SQLLEN, ") FROM STDIN;");
	res = PQexec(conn, &sql[0]);
	if (PQresultStatus(res) != PGRES_COPY_IN) {
		fprintf_s(fpOutP, "Not in COPY_IN mode\n");
	}
	PQclear(res);
	//============================== Data Apply ==============================
	for (i = 0; i < AllHeap[ProdNo].MaxAgntPos + AllHeap[ProdNo].Heap.TotNo.NoSensi; i++) {
		if (AllHeap[ProdNo].Heap.AgentRes[i].K_Need) {//If need to print K-Cash Flow
			sprintf_s(buffer, BUFFLEN, "%d\t%d\t%d\t", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent, AllHeap[ProdNo].Heap.AgentRes[i].Sensi);
			Status = PQputCopyData(conn, buffer, (int)strlen(buffer));
			if (1 != Status) {
				fprintf_s(fpOutP, "PlanID %d inserting error for agent %d\n", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent);
			}
			for (j = 0; j < 8 + AllHeap[ProdNo].Heap.TotNo.NoBenft; j++) {
				if (j == 7 + AllHeap[ProdNo].Heap.TotNo.NoBenft) {
					AsmbCF(buffer, BUFFLEN, ProdNo, i, j, 1);
				}
				else {
					AsmbCF(buffer, BUFFLEN, ProdNo, i, j, 0);
				}
				Status = PQputCopyData(conn, buffer, (int)strlen(buffer));
				if (1 != Status) {
					fprintf_s(fpOutP, "PlanID %d inserting error for agent %d\n", AllHeap[ProdNo].PlanID, AllHeap[ProdNo].Heap.AgentRes[i].Agent);
				}
			}
		}
	}
	Status = PQputCopyEnd(conn, NULL);
#pragma endregion

	PQfinish(conn);
	FreeProd(ProdNo);
	free(buffer);
	end = clock();
	fprintf_s(fpOutP, "PlanID %d inserted, total %d rows inserted, %d millisecond cost\n", AllHeap[ProdNo].PlanID, i, end - begin);
	AllHeap[ProdNo].Printed = 1;
}

