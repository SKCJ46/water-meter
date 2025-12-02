<?php
// =========================================================
//  ESP32-CAM Water Meter -> Cloud OCR (Smart Filter Edition)
// =========================================================

// ===== 1. CONFIGURATION =====
$db_host = "localhost";
$db_user = "xxxxxxxx";
$db_pass = "xxxxxxxx";      // <-- แก้รหัสผ่าน DB
$db_name = "water_meter";
$ocr_api_key = "API Key"; // <-- API Key จาก OCR.space

$upload_dir = "C:/AppServ/www/uploads";

// =========================================================

// เชื่อมต่อฐานข้อมูล
function db_connect() {
    global $db_host, $db_user, $db_pass, $db_name;
    $conn = new mysqli($db_host, $db_user, $db_pass, $db_name);
    if ($conn->connect_error) {
        // ส่ง JSON Error กลับไปทันทีถ้าต่อ DB ไม่ได้
        http_response_code(500);
        die(json_encode(["status" => "error", "message" => "DB Connect Error: " . $conn->connect_error]));
    }
    $conn->set_charset("utf8mb4");
    return $conn;
}

// ฟังก์ชันส่งรูปไป Cloud และกรองตัวเลข (หัวใจสำคัญ)
function run_cloud_ocr_smart($image_path) {
    global $ocr_api_key;

    // เตรียมรูปภาพ
    $file_data = file_get_contents($image_path);
    $base64_image = 'data:image/jpg;base64,' . base64_encode($file_data);

    // ตั้งค่า API (Engine 2 เก่งตัวเลขที่สุด)
    $post_data = array(
        'apikey'            => $ocr_api_key,
        'base64Image'       => $base64_image,
        'language'          => 'eng',
        'isOverlayRequired' => 'false',
        'scale'             => 'true', 
        'OCREngine'         => '2',    
    );

    // ส่ง cURL
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, 'https://api.ocr.space/parse/image');
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    
    $result = curl_exec($ch);
    if(curl_errno($ch)){ return [null, 'Curl error: ' . curl_error($ch)]; }
    curl_close($ch);
    
    // แปลง JSON
    $json = json_decode($result, true);
    
    // ตรวจสอบผลลัพธ์
    if (isset($json['ParsedResults'][0]['ParsedText'])) {
        $raw_text = $json['ParsedResults'][0]['ParsedText'];
        
        // --- SMART FILTER LOGIC (สูตรกรองตัวเลข) ---
        // แม้จะมีหน้ากากปิด แต่เราจะกันเหนียวด้วยการหา Pattern ของเลขมิเตอร์
        
        // 1. ดึงเฉพาะตัวเลขออกมาเป็นชุดๆ (เผื่อมีเลขอื่นหลุดรอดมา)
        preg_match_all('/\d+/', $raw_text, $matches);
        $candidates = $matches[0];
        $best_match = null;

        foreach ($candidates as $num) {
            $len = strlen($num);
            // กฎ: เลขมิเตอร์ควรมียาว 4-8 หลัก (เผื่ออ่านเลขแดงไม่ติด)
            // และมักจะเริ่มด้วย 0 (เช่น 0000097)
            if ($len >= 4 && $len <= 8) {
                // ถ้าขึ้นต้นด้วย 0 ให้คะแนนพิเศษ เลือกทันที
                if (substr($num, 0, 1) === '0') {
                    $best_match = $num;
                    break; 
                }
                // ถ้าไม่ขึ้นต้นด้วย 0 (เช่นอ่านเลขแดงเดี่ยวๆ) เก็บไว้เป็นตัวสำรอง
                if ($best_match === null) {
                    $best_match = $num;
                }
            }
        }
        
        return [$best_match, $raw_text]; // คืนค่าตัวเลขที่ดีที่สุด และข้อความดิบ
        
    } else {
        $err = isset($json['ErrorMessage']) ? $json['ErrorMessage'] : "Unknown API Error";
        return [null, $err];
    }
}

// =========================================================
//  MAIN PROCESS
// =========================================================

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(["status" => "error", "message" => "Method Not Allowed"]);
    exit;
}

$device_id = isset($_GET['device_id']) ? $_GET['device_id'] : 'UNKNOWN';
$rawData = file_get_contents("php://input");

if (!$rawData || strlen($rawData) < 100) {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "No image data"]);
    exit;
}

// ตรวจสอบ/สร้างโฟลเดอร์
if (!is_dir($upload_dir)) mkdir($upload_dir, 0775, true);

// ตั้งชื่อไฟล์
$timestamp = date("Ymd_His");
$filename  = "meter_{$device_id}_{$timestamp}.jpg";
$file_path = rtrim($upload_dir, "/\\") . DIRECTORY_SEPARATOR . $filename;

// บันทึกรูป
if (file_put_contents($file_path, $rawData) === false) {
    http_response_code(500);
    echo json_encode(["status" => "error", "message" => "Save file failed"]);
    exit;
}

// *** เรียกใช้ Smart OCR ***
list($reading_value, $ocr_raw) = run_cloud_ocr_smart($file_path);

// บันทึก DB
$conn = db_connect();
$sql = "INSERT INTO meter_readings (device_id, image_path, reading_value, ocr_raw, created_at) VALUES (?, ?, ?, ?, NOW())";
$stmt = $conn->prepare($sql);
$stmt->bind_param("ssss", $device_id, $file_path, $reading_value, $ocr_raw);

if ($stmt->execute()) {
    $response = [
        "status"   => "ok",
        "id"       => $stmt->insert_id,
        "reading"  => $reading_value, // ค่าที่ผ่านการกรองแล้ว
        "ocr_raw"  => $ocr_raw        // ค่าดิบที่ AI เห็น (เอาไว้ Debug)
    ];
} else {
    $response = ["status" => "error", "message" => "DB Insert Failed"];
}

$stmt->close();
$conn->close();

header("Content-Type: application/json");
echo json_encode($response);
?>