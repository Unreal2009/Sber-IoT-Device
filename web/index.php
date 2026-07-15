<?php
/**
 * dashboard.php — панель данных от ESP32, PHP 7.4
 */

$baseDir = __DIR__;
$uploadDir = $baseDir . '/uploads';
$versionFile = $baseDir . '/version.txt';
$firmwareBin = $baseDir . '/sber_device.bin';

// --- Чтение версии прошивки ---
$currentVersion = 'unknown';
if (file_exists($versionFile)) {
    $currentVersion = trim(file_get_contents($versionFile));
}

// --- Сбор CSV-файлов ---
$csvFiles = [];
if (is_dir($uploadDir)) {
    foreach (glob($uploadDir . '/*.csv') as $path) {
        $name = basename($path);
        $size = filesize($path);
        $mtime = filemtime($path);

        // Быстро считаем строки (не читаем весь файл в память)
        $handle = fopen($path, 'r');
        $lineCount = 0;
        while (fgets($handle)) {
            $lineCount++;
        }
        fclose($handle);

        $csvFiles[] = [
            'name' => $name,
            'size' => $size,
            'lines' => max(0, $lineCount - 1), // минус заголовок
            'mtime' => $mtime,
        ];
    }
}

// --- Парсинг последнего CSV для отображения данных ---
$lastData = [];
$lastCsvName = '';
if (!empty($csvFiles)) {
    // сортируем по времени (новые первыми)
    usort($csvFiles, function ($a, $b) { return $b['mtime'] <=> $a['mtime']; });
    $last = $csvFiles[0];
    $lastCsvName = $last['name'];

    $path = $uploadDir . '/' . $last['name'];
    if (file_exists($path)) {
        if (($handle = fopen($path, 'r')) !== false) {
            $headers = [];
            $rowNum = 0;
            while (($row = fgetcsv($handle, 0, ',')) !== false) {
                if ($rowNum === 0) {
                    // первая строка — заголовки
                    $headers = $row;
                } else {
                    $item = [];
                    foreach ($headers as $i => $h) {
                        $item[$h] = $row[$i] ?? '';
                    }
                    $lastData[] = $item;
                    if (count($lastData) >= 50) break; // ограничиваем вывод
                }
                $rowNum++;
            }
            fclose($handle);
        }
    }
}

// --- Статус наличия прошивки ---
$hasFirmware = file_exists($firmwareBin);
$fwSize = $hasFirmware ? filesize($firmwareBin) : 0;
?>

<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <title>Панель ESP32</title>
    <style>
        body { font-family: system-ui, Segoe UI, Roboto, Arial, sans-serif; margin: 24px; color: #222; background: #f5f7fa; }
        .container { max-width: 1024px; margin: auto; background: white; padding: 24px; border-radius: 8px; box-shadow: 0 2px 8px rgba(0,0,0,0.06); }
        h1, h2 { color: #1a202c; }
        table { width: 100%; border-collapse: collapse; margin-top: 12px; }
        th, td { border: 1px solid #e2e8f0; padding: 8px 12px; text-align: left; }
        th { background: #edf2f7; color: #4a5568; }
        tr:nth-child(even) { background: #f7fafc; }
        .status-ok { color: #2f855a; font-weight: bold; }
        .status-warn { color: #cc4b00; font-weight: bold; }
        .info-block { margin: 16px 0; padding: 12px; background: #fff8e6; border: 1px solid #fbd38d; border-radius: 6px; }
        a { color: #3182ce; text-decoration: none; }
        a:hover { text-decoration: underline; }
        .badge { display: inline-block; padding: 4px 8px; border-radius: 4px; background: #e2e8f0; color: #4a5568; font-size: 0.85em; }
    </style>
</head>
<body>
<div class="container">
    <h1>Панель данных ESP32</h1>

    <!-- Статус прошивки / OTA -->
    <div style="display:flex; gap:16px; flex-wrap:wrap;">
        <div style="flex:1; min-width:240px;">
            <h2>Прошивка (OTA)</h2>
            <p>Текущая версия на сервере: <span class="status-ok"><?= htmlspecialchars($currentVersion) ?></span></p>
            <?php if ($hasFirmware): ?>
                <p class="info-block">
                    Файл прошивки доступен: <strong><?= htmlspecialchars(basename($firmwareBin)) ?></strong><br>
                    Размер: <?= number_format($fwSize) ?> байт<br>
                    URL для OTA: <a href="sber_device.bin">sber_device.bin</a>
                </p>
            <?php else: ?>
                <p class="info-block status-warn">Файл прошивки sber_device.bin не найден в папке сервера.</p>
            <?php endif; ?>
        </div>

        <!-- Список CSV -->
        <div style="flex:1; min-width:240px;">
            <h2>Загруженные CSV</h2>
            <p>Всего файлов: <strong><?= count($csvFiles) ?></strong></p>
            <?php if (empty($csvFiles)): ?>
                <p>Нет загруженных файлов.</p>
            <?php else: ?>
                <table>
                    <thead>
                        <tr>
                            <th>Имя файла</th>
                            <th>Строк</th>
                            <th>Размер</th>
                            <th>Дата</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach (array_slice($csvFiles, 0, 5) as $f): ?>
                            <tr>
                                <td><a href="/uploads/<?= urlencode($f['name']) ?>"><?= htmlspecialchars($f['name']) ?></a></td>
                                <td><?= $f['lines'] ?></td>
                                <td><?= number_format($f['size']) ?> Б</td>
                                <td><?= date('Y-m-d H:i', $f['mtime']) ?></td>
                            </tr>
                        <?php endforeach; ?>
                        <?php if (count($csvFiles) > 5): ?>
                            <tr><td colspan="4" style="text-align:right; color:#718096;">… и ещё <?= count($csvFiles)-5 ?></td></tr>
                        <?php endif; ?>
                    </tbody>
                </table>
            <?php endif; ?>
        </div>
    </div>

    <!-- Данные из последнего CSV -->
    <?php if (!empty($lastData)): ?>
        <h2>Данные из последнего CSV: <?= htmlspecialchars($lastCsvName) ?></h2>
        <p>Показано до 50 последних записей.</p>
        <table>
            <thead>
                <tr>
                    <?php foreach (array_keys($lastData[0]) as $h): ?>
                        <th><?= htmlspecialchars($h) ?></th>
                    <?php endforeach; ?>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($lastData as $row): ?>
                    <tr>
                        <?php foreach ($row as $v): ?>
                            <td><?= htmlspecialchars((string)$v) ?></td>
                        <?php endforeach; ?>
                    </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    <?php else: ?>
        <h2>Нет данных для отображения</h2>
        <p>Загрузите CSV‑файл через ESP32 или вручную, чтобы увидеть данные.</p>
    <?php endif; ?>
</div>
</body>
</html>
