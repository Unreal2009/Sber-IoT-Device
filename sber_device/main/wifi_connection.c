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
#include "esp_tls.h"
#include <esp_http_client.h>
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"

#include "csv.h"
#include "wifi_connection.h"

#include <sys/param.h>

// Путь к файлу
const char *FILE_PATH = "/spiflash/measuring_data.csv";

static const char *SERVER_URL = "http://sber-iot.dcn-rd.ru/upload.php"; // <-- IP сервера

static const char *TAG = "wifi_connection";

// TODO Соптимизировать - чтобы использовать один буфер и для HTTP и для отравки чанков файла
// Буфер для обработки HTTP ответа сервера
#define MAX_HTTP_OUTPUT_BUFFER 2048
static char output_buffer[MAX_HTTP_OUTPUT_BUFFER];

// Буфер для чтения данных файла при отправке
#define CHUNK_SIZE 2048
uint8_t chunk[CHUNK_SIZE];

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

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
    ESP_LOGI(TAG, "Wi-Fi disconnect...");
    return 1;
}

// Отправка данных на сервер
int wifi_send_csv_file()
{
    // Монтируем файловую систему
    if (!csv_mount_fs()) {
        return 0;
    }

    // Открываем файл на чтение
    FILE *f = fopen(FILE_PATH, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s", FILE_PATH);
        csv_unmount_fs();
        return 0;
    }

    // ПОлучаем размер данных
    // TODO Вынести эту ф-цию в утилиты
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);
    ESP_LOGI(TAG, "File size: %zu bytes", file_size);

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .is_async = false,
        .event_handler = _http_event_handler,
    };

    // Инитим клиента
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        fclose(f);
        csv_unmount_fs();
        return 0;
    }

    esp_err_t err = ESP_OK;

    esp_http_client_set_header(client, "Content-Type", "text/csv");

    // Открываем соединение с сервером и отправляем хэдер
    err = esp_http_client_open(client, (int)file_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        fclose(f);
        csv_unmount_fs();
        return 0;
    }

    ESP_LOGI(TAG, "Open HTTP connection OK");

    int return_code = 1;

    // Читаем файл и отправляем на сервер
    while (1) {
        size_t read_size = fread(chunk, 1, CHUNK_SIZE, f);
        // Если файл закончился - выходим из цикла
        if (read_size == 0)
        {
            break;
        }

        ESP_LOGI(TAG, "Read chunk: %zu bytes", read_size);

        int wlen = esp_http_client_write(client, (const char *)chunk, read_size);
        if (wlen < 0 )
        {
            ESP_LOGE(TAG, "Write chunk failed: %s", esp_err_to_name(err));
            return_code = 0;
            goto cleanup;
        }
    }

    // Получаем ответ сервера
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0)
    {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
        // TODO Разобраться что тут делать в такой ситуации
        // return_code = 0;
    }else
    {
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
        if (data_read >= 0)
        {
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
            ESP_LOG_BUFFER_HEX(TAG, output_buffer, strlen(output_buffer));
        } else {
            ESP_LOGE(TAG, "Failed to read response");
            // TODO Разобраться что тут делать в такой ситуации
            return_code = 0;
        }
    }

    // Чистим все и выходит - Добби свободен :)
    cleanup:
        fclose(f);
        csv_unmount_fs();
        // esp_http_client_close(client);
        esp_http_client_cleanup(client);

    return return_code;

}

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

// Обработчик процесса отправки данных - чисто для отладки
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
#if CONFIG_EXAMPLE_ENABLE_RESPONSE_BUFFER_DUMP
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
#endif
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
        default:
            break;
    }
    return ESP_OK;
}