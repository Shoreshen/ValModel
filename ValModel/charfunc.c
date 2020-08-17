#include "Head.h"

void ParseArray(double* Rate, int Dsize, char* Sstr, int tail)
{
	char* p, *nextp, *temp;
	int i, j = 0;
	__int64 Slen = strlen(Sstr);
	if (Slen > 0) {
		temp = malloc(Slen + 1);
		strcpy_s(temp, Slen + 1, Sstr);
		//È¥µô×Ö·ûÀ¨ºÅ
		for (i = 0; i < Slen; i++) {
			if ((temp[i] > 47 && temp[i] < 58) || temp[i] == 44 || temp[i] == 46 || temp[i] == 43 || temp[i] == 45 || temp[i] == 69 || temp[i] == 101) {
				temp[j] = temp[i];
				j++;
			}
		}
		temp[j] = 0;

		p = strtok_s(&temp[0], ",", &nextp);

		if (p != NULL) {
			if (tail == TAILEQPREV)
			{
				for (i = 0; i < Dsize; i++) {
					if (p != NULL) {
						_atodbl((_CRT_DOUBLE*)&Rate[i], p);
						p = strtok_s(NULL, ",", &nextp);
					}
					else {
						Rate[i] = Rate[i - 1];
					}
				}
			}
			else if (tail == TAILNA) {
				i = 0;
				do {
					_atodbl((_CRT_DOUBLE*)&Rate[i], p);
					p = strtok_s(NULL, ",", &nextp);
					i++;
				} while (p != NULL && i < Dsize);
			}
		}
		free(temp);
	}
}

int ParseArrayInt(int* Rate, int Dsize, char* Sstr, int tail)
{
	char* p, *nextp, *temp;
	int i = 0, j = 0;
	__int64 Slen = strlen(Sstr);
	if (Slen > 0) {
		temp = malloc(Slen + 1);
		strcpy_s(temp, Slen + 1, Sstr);
		//È¥µô×Ö·ûÀ¨ºÅ
		for (i = 0; i < Slen; i++) {
			if ((temp[i] > 47 && temp[i] < 58) || temp[i] == 44 || temp[i] == 46) {
				temp[j] = temp[i];
				j++;
			}
		}
		temp[j] = 0;

		p = strtok_s(&temp[0], ",", &nextp);

		if (p != NULL) {
			if (tail == TAILEQPREV)
			{
				for (i = 0; i < Dsize; i++) {
					if (p != NULL) {
						Rate[i] = atoi(p);
						p = strtok_s(NULL, ",", &nextp);
					}
					else {
						Rate[i] = Rate[i - 1];
					}
				}
			}
			else if (tail == TAILNA) {
				i = 0;
				do {
					Rate[i] = atoi(p);
					p = strtok_s(NULL, ",", &nextp);
					i++;
				} while (p != NULL && i < Dsize);
			}
		}
		free(temp);
	}
	return i;
}

void AsmbCF(char *buffer, int size, int ProdNo, int i, int Pos, int LineEnd)
{
	int     j, Step = sizeof(nodecf) / sizeof(double), PosST;
	double *LoopRate = &AllHeap[ProdNo].Heap.AgentRes[i].CF.NodeCF[0].Prem;
	char   *ptr;
	strcpy_s(buffer, size, "{");
	ptr = buffer + 1;
	for (j = 0; j < TOTLEN; j++) {
		PosST = j * Step + Pos;
		ptr = dbltoa(ptr, LoopRate[PosST], PRECI);
		ptr[0] = ',';
		ptr++;
	}
	ptr[-1] = 0;
	if (LineEnd) {
		strcat_s(buffer, size, "}\n");
	}
	else {
		strcat_s(buffer, size, "}\t");
	}
}

__inline char* dbltoa(char* buff, double A, int Precision)
{
	int     Temp, i;
	char   *ptr;
	double  Decimal;

	Temp = (int)A;
	_itoa_s(Temp, buff, MAXILEN, 10);
	ptr = buff + strlen(buff);
	ptr[0] = '.';
	ptr++;
	Decimal = A - Temp;
	for (i = 1; i < Precision; i++) {
		Decimal = Decimal * 10;
		Temp = (int)Decimal;
		Decimal = Decimal - Temp;
		ptr[0] = NumRep[Temp];
		ptr++;
	}
	return ptr;
}