#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"

#include "csv_worker.h"

static const char *TAG = "csv_worker";

// Mount path for the partition
const char *base_path = "/spiflash";

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

static const char *filename = "/spiflash/measuring_data.csv";


// Ф-ция для вычисления размера файла
static size_t get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return (size_t)st.st_size;
    }
    ESP_LOGE(TAG, "stat failed for %s", path);
    return 0;
}

// Монтируем файловую систему
int csv_worker_mount_fs()
{
    // Монтируем файловую систему
    // ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4, // Number of files that can be open at a time
        .format_if_mount_failed = true, // If true, try to format the partition if mount fails
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE, // Size of allocation unit, cluster size.
        .use_one_fat = false, // Use only one FAT table (reduce memory usage), but decrease reliability of file system in case of power failure.
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return 0;
    }

    // ESP_LOGI(TAG, "Filesystem mounted");

    return 1;
}

// Размонтируем файловую систему
int csv_worker_unmount_fs()
{
    // Unmount FATFS
    // Можно не делать это - но незачем висеть ресурсам и при сбросе питания может все поломаться.
    // ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount_rw_wl(base_path, s_wl_handle));
    return 1;
}


// Запись данных в CSV файл
int csv_worker_create_file(measuring_data_t measuring_data)
{
    if (measuring_data.data_size == 0)
    {
        ESP_LOGI(TAG, "Internal error - empty data");
        return 0;
    }

    if (!csv_worker_mount_fs())
    {
        return 0;
    }


    // Открываем файл на запись и пишем данные
    ESP_LOGI(TAG, "Opening file");

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        perror("fopen"); // Print reason why fopen failed
        ESP_LOGE(TAG, "Failed to open file for writing");
        csv_worker_unmount_fs();
        return 0;
    }

    fprintf(f, "index,value\n");
    for (int i = 0; i < measuring_data.data_size; i++)
    {
        fprintf(f, "%d,%d\n", i, measuring_data.data[i]);
    }


    fclose(f);

    ESP_LOGI(TAG, "CSV created");
    ESP_LOGI(TAG, "CSV size: %d bytes", get_file_size(filename));

    csv_worker_unmount_fs();

    return 1;
}

// Выводит в лог немного данных из файла
int csv_worker_preview()
{
    if (!csv_worker_mount_fs())
    {
        return 0;
    }

    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        perror("fopen"); // Print reason why fopen failed
        ESP_LOGE(TAG, "Failed to open file for reading");
        csv_worker_unmount_fs();
        return 0;
    }

    char line[128];
    int line_index = 0;
    int line_total = 0;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "CSV preview:");

    // Считаем количество строк в файле
    while (fgets(line, sizeof(line), f))
    {
        line_total++;
    }

    // Сдвигаем указатель чтения на начало
    fseek(f, 0, SEEK_SET);

    // Проходимся по файлу и выводим красоту
    while (fgets(line, sizeof(line), f))
    {
        // strip newline
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }

        if (line_index < 4 || line_index > line_total - 4)
        {
            ESP_LOGI(TAG, "%s", line);
        }else if (line_index == 4)
        {
            ESP_LOGI(TAG, "...");
        }

        line_index++;
    }

    fclose(f);

    csv_worker_unmount_fs();
    return 1;
}

void csv_worker_test(void)
{
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formatted before
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4, // Number of files that can be open at a time
            .format_if_mount_failed = true, // If true, try to format the partition if mount fails
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE, // Size of allocation unit, cluster size.
            .use_one_fat = false, // Use only one FAT table (reduce memory usage), but decrease reliability of file system in case of power failure.
    };

    // Mount FATFS filesystem located on "storage" partition in read-write mode
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    ESP_LOGI(TAG, "Opening file");

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        perror("fopen"); // Print reason why fopen failed
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "index,value\n");
    fclose(f);

    ESP_LOGI(TAG, "File written");

    // Open file for reading
    ESP_LOGI(TAG, "Reading file");

    f = fopen(filename, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    char line[128];

    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }

    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // Unmount FATFS
    ESP_LOGI(TAG, "Unmounting FAT filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount_rw_wl(base_path, s_wl_handle));

    ESP_LOGI(TAG, "Done");
}
