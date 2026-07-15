#ifndef _CSV_WORKER_H_
#define _CSV_WORKER_H_

#include "measuring.h"

// Тест файловой системы
// void csv_worker_test(void);

// Запись данных в CSV файл
int csv_worker_create_file(measuring_data_t measuring_data);

// Выводит в лог немного данных из файла
int csv_worker_preview();

#endif //_CSV_WORKER_H_