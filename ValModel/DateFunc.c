#include "Head.h"

int DaysOfMonth(int year, int month)
{
	switch (month)
	{
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	case 2:
		if (year % 400 == 0 || year % 4 == 0 && year % 100 != 0)
			return 29;
		else
			return 28;
	default:
		return 31;
	}
}