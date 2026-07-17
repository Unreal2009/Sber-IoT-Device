#ifndef ESP_HTTP_CLIENT_WIFI_CONNECTION_H
#define ESP_HTTP_CLIENT_WIFI_CONNECTION_H


// Подключение к WiFi
int wifi_connect();

// Отключение от WiFi
int wifi_disconnect();

// Отправка данных на сервер
int wifi_send_csv_file();

#endif //ESP_HTTP_CLIENT_WIFI_CONNECTION_H