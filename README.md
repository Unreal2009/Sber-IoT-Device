# Sber-IoT-Device
Проект на базе ESP32-S3 + FreeRTOS для получения данных с АЦП и передачи их на сервер

## Оборудование

Используется плата:
```
ESP32-S3-DevKitC-1 N16R8
16 MB Flash
8 MB PSRAM
USB
Wi-Fi
BLE
```

К плате подключены:

* RGB-светодиод (встроенный);
* DytRGB-светодиод (встроенный);
* кнопка (GPIO_47);
* генератор сигнала на вход ADC.

SSID и password Wi-Fi можно задаются в коде.

## Памятка - настройка и сборка проекта

```
idf.py create-project
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p COMx flash monitor
```