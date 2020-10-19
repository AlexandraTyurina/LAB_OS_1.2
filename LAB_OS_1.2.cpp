#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <iostream>
#include <string>
#include <time.h>
#pragma comment (lib, "winmm.lib")

#define buffer_size 4096
DWORD bytes_transferred = 0;
/*
Функция FileIOCompletionRoutine - определяемая программой функция обратного вызова, используемая с функциями ReadFileEx и WriteFileEx. 
Она вызывается тогда, когда операция асинхронного ввода и вывода данных завершается или отменяется, а вызывающий поток находится в состоянии готовности.
FileIOCompletionRoutine - символ-заместитель для определяемого программой  названия функции.
*/
VOID CALLBACK FileIOCompletionRoutine(__in  DWORD ErrorCode, __in  DWORD NumberOfBytesTransfered, __in  LPOVERLAPPED lpOverlapped) {
	bytes_transferred = NumberOfBytesTransfered;
}

int main()
{
	setlocale(LC_ALL, "Russian");
	int N = 0;
	std::cout << "Введите количество перекрывающих операций: ";
	std::cin >> N;
	std::wstring path_to_read, path_to_write;
	std::cout << "Путь для чтения: ";
	std::wcin >> path_to_read;
	std::cout << "Путь для записи: ";
	std::wcin >> path_to_write;
	DWORD timer1, timer2;

	HANDLE file_hadle_1;
	HANDLE file_hadle_2;
	int count = 0;
	int count_1 = 0;
	int stop = 0;
	char buffer[16][buffer_size] = { 0 };
	OVERLAPPED over = { 0 };
	
	file_hadle_1 = CreateFile(path_to_read.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
	if (file_hadle_1 == INVALID_HANDLE_VALUE)
	{
		std::cout << "Ошибка чтения!" << std::endl;
		return 0;
	}

	file_hadle_2 = CreateFile(path_to_write.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
	if (file_hadle_2 == INVALID_HANDLE_VALUE)
	{
		std::cout << "Ошибка записи!" << std::endl;
		return 0;
	}
	
	DWORD bytes;
	while (true)
	{
		timer1 = timeGetTime();

		for (int i = 0; i < N; i++) {
			count_1 = count;
			over.Offset = count_1 * buffer_size;
			/*Функция ReadFileEx читает данные из файла асинхронно. Она предназначена исключительно для асинхронных операций. 
			ReadFileEx позволяет приложению в ходе операции чтения файла исполнять другую работу. 
			*/
			if (FALSE == ReadFileEx(file_hadle_1, buffer[i], buffer_size, &over, FileIOCompletionRoutine))
			{
				std::cout << "Ошибка чтения!" << std::endl;
				return 0;
			}
			/*Функция SleepEx приостанавливает исполнение текущего потока до тех пор, пока не произойдет одно иp перечисленного:
				1)Вызывается функция повторного вызова, завершившая ввод-вывод
				2)Асинхронный вызов процедуры (APC) ставится в очередь потока.
				3)Истекает минимальный интервал времени перерыва
			*/
			SleepEx(50000, TRUE);
			if (bytes_transferred < buffer_size) {
				bytes = bytes_transferred;
				stop = i;
				break;
			}
			stop = N;
			count_1++;
		}
		if (stop >= 0 && stop < N) {
			count_1 = count;
			for (int i = 0; i <= stop; i++) {
				over.Offset = (count_1)*buffer_size;
				/*Функция WriteFileEx пишет данные в файл. 
				Она сообщает о своем состоянии завершения асинхронно, вызывая заданную процедуру завершения, когда запись завершается или отменяется, а вызывающий поток находится в готовом к действию режиме ожидания.				
				*/
				if (WriteFileEx(file_hadle_2, buffer[i], buffer_size, &over, FileIOCompletionRoutine) == FALSE) {
					std::cout << "Ошибка записи!" << std::endl;
					return 0;
				}
				count_1++;
			}
			bytes = bytes_transferred;
			//Функция SetFilePointer перемещает указатель позиции в открытом файле.
			SetFilePointer(file_hadle_2, over.Offset + bytes, NULL, FILE_BEGIN);
			break;
		}
		else {
			count_1 = count;
			for (int i = 0; i < stop; i++) {
				over.Offset = count_1 * buffer_size;
				if (WriteFileEx(file_hadle_2, buffer[i], buffer_size, &over, FileIOCompletionRoutine) == FALSE) {
					std::cout << "Ошибка записи!" << std::endl;
					return 0;
				}
				count_1++;
			}
		}
		SleepEx(50000, TRUE);
		count = count + N;
	}
	/*Функция SetEndOfFile перемещает  позицию метки конца файла (EOF) для заданного файла к текущую позицию его указателя.
	Эта функция устанавливает физический конец файла (как указывается распределенными кластерами).
	*/
	SetEndOfFile(file_hadle_2);

	CloseHandle(file_hadle_1);
	CloseHandle(file_hadle_2);
	timer2 = timeGetTime();
	std::cout << "Время в секундах: ";
	std::cout << ((double)timer2 - (double)timer1)/1000 << std::endl;
	std::cout << "Копирование выполнено успешно!" << std::endl;
	std::cout << "Count: ";
	std::cout << count << std::endl;
	system("pause");
	return 0;
}
