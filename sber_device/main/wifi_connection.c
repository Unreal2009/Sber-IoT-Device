#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "sdkconfig.h"

#include <esp_http_client.h>
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"

#include "csv.h"
#include "wifi_connection.h"



// Путь к файлу
const char *FILE_PATH = "/spiflash/measuring_data.csv";

static const char *SERVER_URL = "http://sber-iot.dcn-rd.ru/upload.php"; // <-- IP сервера

static const char *TAG = "wifi_connection";

static esp_err_t send_csv_file_stream(void);

// Подключение к WiFi
int wifi_connect()
{
    static int is_init = 0;

    if (!is_init)
    {
        // System initialization
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        if (ret != ESP_OK)
        {
            return 0;
        };

        if (esp_netif_init() != ESP_OK)
        {
            return 0;
        };

        if(esp_event_loop_create_default() != ESP_OK)
        {
            return 0;
        }
        is_init = 1;
    }

    ESP_LOGI(TAG, "Starting Wi-Fi connect...");

    // ждёт подключения, блокирует до успеха или ошибки
    if(example_connect() != ESP_OK)
    {
        return 0;
    }
    ESP_LOGI(TAG, "Wi-Fi connected");

    return 1;
}

// Отключение от WiFi
int wifi_disconnect()
{
    ESP_ERROR_CHECK(example_disconnect());
    return 1;
}

// Отправка данных на сервер
int wifi_send_csv_file()
{
    const char *boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    const char *filename = "measurements.csv";

    csv_mount_fs();

    FILE *f = fopen(FILE_PATH, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s", FILE_PATH);
        csv_unmount_fs();
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);
    ESP_LOGI(TAG, "File size: %zu bytes", file_size);

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .event_handler = NULL,
        // .is_keep_open = true,          // <--- ВАЖНО: разрешаем писать частями
        .timeout_ms = 10000           // таймаут на операцию
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        fclose(f);
        csv_unmount_fs();
        return 0;
    }

    // Устанавливаем заголовок Content-Type
    char content_type[128];
    snprintf(content_type, sizeof(content_type),
             "multipart/form-data; boundary=%s", boundary);
    esp_http_client_set_header(client, "Content-Type", content_type);

    // 1. Пишем начало multipart
    char header_buf[256];
    int h_len = snprintf(header_buf, sizeof(header_buf),
                         "--%s\r\n"
                         "Content-Disposition: form-data; name=\"csv_file\"; filename=\"%s\"\r\n"
                         "Content-Type: text/csv\r\n\r\n",
                         boundary, filename);

    esp_err_t err = esp_http_client_write(client, header_buf, h_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write header failed: %s", esp_err_to_name(err));
        fclose(f);
        csv_unmount_fs();
        esp_http_client_cleanup(client);
        return 0;
    }

    // 2. Поблочно пишем файл
    const size_t CHUNK_SIZE = 1024;
    uint8_t chunk[CHUNK_SIZE];

    while (1) {
        size_t read_size = fread(chunk, 1, CHUNK_SIZE, f);
        if (read_size == 0) break;

        err = esp_http_client_write(client, (const char *)chunk, read_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Write chunk failed: %s", esp_err_to_name(err));
            break;
        }
    }
    fclose(f);
    csv_unmount_fs();

    // 3. Пишем конец multipart
    char footer_buf[128];
    int f_len = snprintf(footer_buf, sizeof(footer_buf), "\r\n--%s--\r\n", boundary);
    err = esp_http_client_write(client, footer_buf, f_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write footer failed: %s", esp_err_to_name(err));
    }

    // 4. Только теперь завершаем запрос
    err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);

    ESP_LOGI(TAG, "HTTP POST status=%d, err=%d", status_code, err);

    esp_http_client_cleanup(client);
    return 1;
}