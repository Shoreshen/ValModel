#include "Head.h"

#pragma region 全局变量
glbAss    GlobAsmp;
allHeap  *AllHeap;
FILE     *fpAsmp, *fpOutP, *fpMain;
char      NumRep[11] = "0123456789";
#pragma endregion

int main()
{
	HANDLE    THandle[NOTHRED];
	time_t    t;
	struct tm lt;
	clock_t   begin, end;
	char      TempChar[SQLLEN];
	int       i, Thread;
	PGresult *res;
	//system("chcp 65001");//Set to unicode page
#pragma region Initialization
	AllHeap = NULL;
	memset(&GlobAsmp, 0, sizeof(glbAss));
	//============================== Val date ==============================
	time(&t);
	localtime_s(&lt, &t);
	GlobAsmp.ValDate = (lt.tm_year + 1900) * 10000 + lt.tm_mon * 100 + DaysOfMonth(lt.tm_year + 1900, lt.tm_mon);
	GlobAsmp.ValMon  = (lt.tm_year + 1900) * 100 + lt.tm_mon;
	GlobAsmp.ValM    = lt.tm_mon;
	GlobAsmp.ValY    = lt.tm_year + 1900;
	//============================== Log file ==============================
	sprintf_s(&TempChar[0], SQLLEN, "Log_Asmp_%d.txt", GlobAsmp.ValDate);
	fopen_s(&fpAsmp, &TempChar[0], "w");
	sprintf_s(&TempChar[0], SQLLEN, "Log_OutP_%d.txt", GlobAsmp.ValDate);
	fopen_s(&fpOutP, &TempChar[0], "w");
	sprintf_s(&TempChar[0], SQLLEN, "Log_Main_%d.txt", GlobAsmp.ValDate);
	fopen_s(&fpMain, &TempChar[0], "w");
#pragma endregion

#pragma region Start up thread loading assumptions
	fprintf_s(fpMain, "Number of thread applied: %d\n", NOTHRED);
	fprintf_s(fpMain, "Valuation date at: %d\n", GlobAsmp.ValDate);
	fprintf_s(fpMain, "--------------------------------------------------------------------\n");
	fprintf_s(fpMain, "Connecting database...\n");
	if (DBConn(&GlobAsmp.conn, CONNSTR, fpMain)) { 
		printf("Data base connect failure!");
		system("pause");  
		return 0; 
	};
	fprintf_s(fpMain, "Clearing last month content...\n");
	if (DBExec(GlobAsmp.conn, &res, "Truncate Table AgentCF;",   fpAsmp)) {
		printf("AgentCF truncate failure!");
		system("pause"); 
		return; 
	}
	if (DBExec(GlobAsmp.conn, &res, "Truncate Table AgentCF_K;", fpAsmp)) { 
		printf("AgentCF_K truncate failure!");
		system("pause"); 
		return; 
	}
	fprintf_s(fpMain, "Loading global assumption...\n");
	if (LoadGlable()) { return 0; }
	fprintf_s(fpMain, "Start thread loading product assumption...\n");
	_beginthread(ThreadLoadProd, 0, NULL);
#pragma endregion

#pragma region Calculation loop
	for (i = 0; i < GlobAsmp.NoProd; i++) {
		RETRY:
		if (AllHeap[i].Loaded != 1) { goto RETRY; }
		fprintf_s(fpMain, "====================================================================\n");
		fprintf_s(fpMain, "Start calculation for product: %d\n", AllHeap[i].PlanID);
		begin = clock();
		if (AllHeap[i].Heap.NoPol > 0) {
			for (Thread = 0; Thread < NOTHRED; Thread++) {
				AllHeap[i].TInfo[Thread].ProdNo = i;
				THandle[Thread] = (HANDLE)_beginthread(ThreadCalc, 0, (LPVOID)&AllHeap[i].TInfo[Thread]);
			}
			WaitForMultipleObjects(NOTHRED, &THandle[0], TRUE, INFINITE);
			_beginthread(ThreadOutP, 0, (LPVOID)i);
		}
		else {
			AllHeap[i].Printed = 1;
		}
		end = clock();
		fprintf_s(fpMain, "Total cost: %d millisecond\n", end - begin);
		fprintf_s(fpMain, "--------------------------------------------------------------------\n");
	}
#pragma endregion
	
#pragma region Ensure threads finished
KeepWait:
	for (i = 0; i < GlobAsmp.NoProd; i++) {
		if (!AllHeap[i].Printed || !AllHeap[i].Loaded) {
			goto KeepWait;
		}
	}
#pragma endregion

#pragma region Disconnect & free
	free(AllHeap);
	fclose(fpAsmp);
	fclose(fpOutP);
	fclose(fpMain);
	PQfinish(GlobAsmp.conn);
#pragma endregion
	printf("End of the Process!\n");
	system("pause");
	return 0;
}