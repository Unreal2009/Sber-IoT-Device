<?php
/**
 * index.php — панель данных от ESP32, PHP 7.4
 * Автообновление: если появилось/исчезло CSV — полная перезагрузка страницы
 */

$baseDir = __DIR__;
$uploadDir = $baseDir . '/uploads';
$versionFile = $baseDir . '/version.txt';
$firmwareBin = $baseDir . '/sber_device.bin';

// --- Обработка действия: удалить все CSV ---
if ($_SERVER['REQUEST_METHOD'] === 'POST' && ($_POST['action'] ?? '') === 'delete_all_csv') {
    if (is_dir($uploadDir)) {
        foreach (glob($uploadDir . '/*.csv') as $path) {
            if (is_file($path)) {
                unlink($path);
            }
        }
    }
    header('Location: ' . $_SERVER['PHP_SELF']);
    exit;
}

// --- Эндпоинт: только JSON (счётчик CSV) ---
if (isset($_GET['check_updates']) && $_GET['check_updates'] == '1') {
    header('Content-Type: application/json');
    $count = 0;
    if (is_dir($uploadDir)) {
        $count = count(glob($uploadDir . '/*.csv'));
    }
    echo json_encode(['csv_count' => $count]);
    exit;
}

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

        $handle = fopen($path, 'r');
        $lineCount = 0;
        while (fgets($handle)) {
            $lineCount++;
        }
        fclose($handle);

        $csvFiles[] = [
            'name' => $name,
            'size' => $size,
            'lines' => max(0, $lineCount - 1),
            'mtime' => $mtime,
        ];
    }
}
usort($csvFiles, function ($a, $b) { return $b['mtime'] <=> $a['mtime']; });

// --- Парсинг последнего CSV для отображения данных ---
$lastData = [];
$lastCsvName = '';
if (!empty($csvFiles)) {
    $last = $csvFiles[0];
    $lastCsvName = $last['name'];

    $path = $uploadDir . '/' . $last['name'];
    if (file_exists($path)) {
        if (($handle = fopen($path, 'r')) !== false) {
            $headers = [];
            $rowNum = 0;
            while (($row = fgetcsv($handle, 0, ',')) !== false) {
                if ($rowNum === 0) {
                    $headers = $row;
                } else {
                    $item = [];
                    foreach ($headers as $i => $h) {
                        $item[$h] = $row[$i] ?? '';
                    }
                    $lastData[] = $item;
                    if (count($lastData) >= 50) break;
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
    <title>Панель данных ESP32</title>
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

        .delete-all-form { margin-top: 8px; }
        .btn-delete-all {
            background: #e53e3e; color: white; border: none; padding: 8px 16px;
            border-radius: 6px; cursor: pointer; font-size: 14px;
        }
        .btn-delete-all:hover { background: #c53030; }

        #auto-refresh-status { margin-top: 12px; font-size: 13px; color: #718096; }
        #auto-refresh-status.checking { color: #4299e1; }
        #auto-refresh-status.updated { color: #2f855a; }
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

            <?php if (!empty($csvFiles)): ?>
                <form method="POST" class="delete-all-form" onsubmit="return confirm('Вы уверены, что хотите удалить ВСЕ CSV‑файлы?');">
                    <input type="hidden" name="action" value="delete_all_csv">
                    <button type="submit" class="btn-delete-all">Удалить все CSV</button>
                </form>
            <?php endif; ?>

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

    <div id="auto-refresh-status">Автообновление: выключено (ожидание JS)</div>
</div>

<script>
(function() {
    const statusEl = document.getElementById('auto-refresh-status');
    let lastCount = <?= json_encode(count($csvFiles)); ?>;

    function checkForUpdates() {
        statusEl.textContent = 'Автообновление: проверка…';
        statusEl.className = 'checking';

        fetch('index.php?check_updates=1')
            .then(response => response.json())
            .then(data => {
                const currentCount = data.csv_count ?? 0;
                if (currentCount !== lastCount) {
                    statusEl.textContent = `Автообновление: обнаружено изменение (было ${lastCount}, стало ${currentCount}) — перезагрузка…`;
                    statusEl.className = 'updated';
                    setTimeout(() => window.location.reload(), 1500); // даём пользователю увидеть сообщение
                } else {
                    statusEl.textContent = `Автообновление: всё актуально (файлов: ${currentCount})`;
                    statusEl.className = '';
                }
                lastCount = currentCount;
            })
            .catch(err => {
                console.error('Ошибка проверки обновлений:', err);
                statusEl.textContent = 'Автообновление: ошибка проверки';
                statusEl.className = '';
            });
    }

    // Запускаем сразу, потом каждые 10 секунд (10000 мс)
    checkForUpdates();
    setInterval(checkForUpdates, 10000);
})();
</script>
</body>
</html>
