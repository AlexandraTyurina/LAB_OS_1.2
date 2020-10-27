#include <iostream>

#include <conio.h>

#include <Windows.h>

#pragma comment(lib, "winmm.lib")


int BLOCK_SIZE, MAX_OVR;

HANDLE file_hadle_1, file_hadle_2;


OVERLAPPED* ovrIn, * ovrOut;

CHAR** buf;

LONGLONG nRecords, nDoneRead, nDoneWrite;
/*Структура LARGE_INTEGER используется для представления 64 - х битного знакового целочислительного значения.
typedef union _LARGE_INTEGER {
	struct {
		DWORD LowPart;  - определяет младшее 32 бита
		LONG  HighPart; - определяет старшие 32 бита
	};
	LONGLONG QuadPart;  - определяет 64-х битное знаковое целое
} LARGE_INTEGER;
*/
LARGE_INTEGER fileSize;



VOID WINAPI ReadCallback(DWORD error, DWORD countOfBytes, LPOVERLAPPED pOvr);

VOID WINAPI WriteCallback(DWORD error, DWORD countOfBytes, LPOVERLAPPED pOvr);


int main()

{

	DWORD start, finish;

	setlocale(LC_ALL, "Russian");

	std::wstring path_to_read, path_to_write;
	std::cout << "Путь для чтения: ";
	std::wcin >> path_to_read;
	std::cout << "Путь для записи: ";
	std::wcin >> path_to_write;

	file_hadle_1 = CreateFile(path_to_read.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
	file_hadle_2 = CreateFile(path_to_write.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);

	if ((file_hadle_1 != INVALID_HANDLE_VALUE) && (file_hadle_2 != INVALID_HANDLE_VALUE))

		{
			LARGE_INTEGER position;
			std::cout << "\nВведите размер копируемых блоков: ";
			std::cin >> BLOCK_SIZE;
			std::cout << "Введите число перекрывающихся операций: ";
			std::cin >> MAX_OVR;
			
			buf = new CHAR * [MAX_OVR];
			for (int k = 0; k < MAX_OVR; k++)
			{
				buf[k] = new CHAR[BLOCK_SIZE];
			}

			ovrIn = new OVERLAPPED[MAX_OVR];
			ovrOut = new OVERLAPPED[MAX_OVR];

			BY_HANDLE_FILE_INFORMATION fileInformation;

			if (!GetFileInformationByHandle(file_hadle_1, &fileInformation))
			{
				std::cout << "Ошибка извлечения информации!";
				return 0;
			}

			fileSize.LowPart = (fileInformation.nFileSizeHigh * (MAXDWORD + 1)) + fileInformation.nFileSizeLow;
			nRecords = fileSize.QuadPart / BLOCK_SIZE + (fileSize.QuadPart % BLOCK_SIZE > 0 ? 1 : 0);
			position.QuadPart = 0;
			int i;
			start = timeGetTime();
			for (i = 0; i < MAX_OVR; i++)
			{
				ovrIn[i].hEvent = (HANDLE)i;
				ovrOut[i].hEvent = (HANDLE)i;
				ovrIn[i].Offset = position.LowPart;
				ovrIn[i].OffsetHigh = position.HighPart;
				if (position.QuadPart < fileSize.QuadPart)
				{
					ReadFileEx(file_hadle_1, buf[i], BLOCK_SIZE, &ovrIn[i], ReadCallback);
				}
				position.QuadPart = position.QuadPart + (LONGLONG)BLOCK_SIZE;
			}
			nDoneRead = 0;
			while (nDoneRead < nRecords)
			{
				SleepEx(INFINITE, TRUE);
			}
			LONGLONG nBytes = nRecords * BLOCK_SIZE;

			SetFilePointer(file_hadle_2, -(nBytes - fileSize.QuadPart), NULL, FILE_END);
			SetEndOfFile(file_hadle_2);

			finish = timeGetTime();

			std::cout << "\nВремя копирования в миллисекундах: " << (double)finish - (double)start << std::endl;

			CloseHandle(file_hadle_1);
			CloseHandle(file_hadle_2);

			delete[] ovrIn;
			delete[] ovrOut;
			for (int i = 0; i < MAX_OVR; i++)
			{
				delete[] buf[i];
			}
			delete[] buf;

		}
		else
		std::cout << "\nОшибка открытия файла\n";

	_getch();
	return 0;
}


VOID CALLBACK ReadCallback(DWORD error, DWORD countOfBytes, LPOVERLAPPED pOvr)

{
	LARGE_INTEGER positionIN;
	nDoneRead++;
	DWORD k = (DWORD)(pOvr->hEvent);
	positionIN.LowPart = ovrIn[k].Offset;
	positionIN.HighPart = ovrIn[k].OffsetHigh;
	ovrOut[k].Offset = ovrIn[k].Offset;
	ovrOut[k].OffsetHigh = ovrIn[k].OffsetHigh;
	WriteFileEx(file_hadle_2, buf[k], BLOCK_SIZE, &ovrOut[k], WriteCallback);
	positionIN.QuadPart += BLOCK_SIZE * (LONGLONG)(MAX_OVR);
	ovrIn[k].Offset = positionIN.LowPart;
	ovrIn[k].OffsetHigh = positionIN.HighPart;
	return;
}

VOID CALLBACK WriteCallback(DWORD error, DWORD countOfBytes, LPOVERLAPPED pOvr)
{
	LARGE_INTEGER position;
	nDoneWrite++;
	DWORD k = (DWORD)(pOvr->hEvent);
	position.LowPart = ovrIn[k].Offset;
	position.HighPart = ovrIn[k].OffsetHigh;
	if (position.QuadPart < fileSize.QuadPart)
	{
		ReadFileEx(file_hadle_1, buf[k], BLOCK_SIZE, &ovrIn[k], ReadCallback);
	}
	return;

}