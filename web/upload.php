<?php
$upload_dir = __DIR__ . '/uploads';
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0777, true);
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['status' => 'error', 'message' => 'Use POST']);
    exit;
}

$file_key = 'csv_file'; // ключ, который ESP32 будет слать в поле form-data

if (!isset($_FILES[$file_key]) || $_FILES[$file_key]['error'] !== UPLOAD_ERR_OK) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'No file or upload error',
        'details' => $_FILES[$file_key] ?? null
    ]);
    exit;
}

$tmp_name = $_FILES[$file_key]['tmp_name'];
$orig_name = basename($_FILES[$file_key]['name']);
$dest_path = $upload_dir . '/' . date('Y-m-d_H-i-s_') . $orig_name;

if (move_uploaded_file($tmp_name, $dest_path)) {
    http_response_code(200);
    echo json_encode([
        'status' => 'ok',
        'file' => $dest_path,
        'size' => filesize($dest_path)
    ]);
} else {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'message' => 'Failed to save file']);
}
