#ifndef _CSV_WORKER_H_
#define _CSV_WORKER_H_

#include "measuring.h"

// Тест файловой системы
// void csv_worker_test(void);

// Запись данных в CSV файл
int csv_create_file(measuring_data_t measuring_data);

// Выводит в лог немного данных из файла
int csv_preview();

// Монтируем файловую систему
int csv_mount_fs();

// Размонтируем файловую систему
int csv_unmount_fs();

#endif //_CSV_WORKER_H_