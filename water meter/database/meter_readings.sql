-- phpMyAdmin SQL Dump
-- version 4.9.1
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Dec 02, 2025 at 03:45 AM
-- Server version: 8.0.17
-- PHP Version: 7.3.10

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `water_meter`
--

-- --------------------------------------------------------

--
-- Table structure for table `meter_readings`
--

CREATE TABLE `meter_readings` (
  `id` int(11) NOT NULL,
  `device_id` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `image_path` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `ocr_raw` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `reading_value` float(10,2) DEFAULT NULL,
  `created_at` datetime NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Dumping data for table `meter_readings`
--

INSERT INTO `meter_readings` (`id`, `device_id`, `image_path`, `ocr_raw`, `reading_value`, `created_at`) VALUES
(70, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_123501.jpg', '', NULL, '2025-12-01 12:35:01'),
(71, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_123609.jpg', '', NULL, '2025-12-01 12:36:09'),
(72, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_123714.jpg', '', NULL, '2025-12-01 12:37:14'),
(73, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_125802.jpg', '', NULL, '2025-12-01 12:58:02'),
(74, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_131418.jpg', '', NULL, '2025-12-01 13:14:18'),
(75, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_133922.jpg', '', NULL, '2025-12-01 13:39:22'),
(76, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_134038.jpg', '', NULL, '2025-12-01 13:40:38'),
(77, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_134149.jpg', '', NULL, '2025-12-01 13:41:49'),
(78, 'ESP32CAM_01', 'C:/Users/LENOVO/Desktop/water meter/Pic_nodered/ESP32CAM_01_20251201_134554.jpg', '', NULL, '2025-12-01 13:45:54'),
(79, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_064911.jpg', '', NULL, '2025-12-01 13:49:11'),
(80, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_064936.jpg', '', NULL, '2025-12-01 13:49:36'),
(81, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065029.jpg', '', NULL, '2025-12-01 13:50:29'),
(82, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065041.jpg', '', NULL, '2025-12-01 13:50:41'),
(83, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065057.jpg', '', NULL, '2025-12-01 13:50:57'),
(84, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065131.jpg', '', NULL, '2025-12-01 13:51:31'),
(85, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065156.jpg', '', NULL, '2025-12-01 13:51:56'),
(86, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_065221.jpg', '', NULL, '2025-12-01 13:52:21'),
(87, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_070543.jpg', '', NULL, '2025-12-01 14:05:43'),
(88, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_071739.jpg', '', NULL, '2025-12-01 14:17:39'),
(89, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_071832.jpg', '', NULL, '2025-12-01 14:18:32'),
(90, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_071943.jpg', '', NULL, '2025-12-01 14:19:43'),
(91, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_072118.jpg', '', NULL, '2025-12-01 14:21:18'),
(92, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_072130.jpg', '', NULL, '2025-12-01 14:21:30'),
(93, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_072243.jpg', '', NULL, '2025-12-01 14:22:44'),
(94, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_073426.jpg', '', NULL, '2025-12-01 14:34:26'),
(95, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_073647.jpg', '', NULL, '2025-12-01 14:36:47'),
(96, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074013.jpg', '', NULL, '2025-12-01 14:40:13'),
(97, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074120.jpg', '', NULL, '2025-12-01 14:41:20'),
(98, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074356.jpg', '', NULL, '2025-12-01 14:43:56'),
(99, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074459.jpg', '', NULL, '2025-12-01 14:45:00'),
(100, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074604.jpg', '', NULL, '2025-12-01 14:46:04'),
(101, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074708.jpg', '', NULL, '2025-12-01 14:47:08'),
(102, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074816.jpg', '', NULL, '2025-12-01 14:48:17'),
(103, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_074920.jpg', '', NULL, '2025-12-01 14:49:20'),
(104, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_075028.jpg', '', NULL, '2025-12-01 14:50:28'),
(105, 'ESP32CAM_01', 'uploads/meter_ESP32CAM_01_20251201_075137.jpg', '', NULL, '2025-12-01 14:51:37'),
(106, 'ESP32CAM_01', 'C:/AppServ/www/uploads\\meter_ESP32CAM_01_20251201_090934.jpg', 'ImageMagick failed (Check path or permissions)', NULL, '2025-12-01 16:09:34'),
(107, 'ESP32CAM_01', 'C:/AppServ/www/uploads\\meter_ESP32CAM_01_20251201_095029.jpg', 'ImageMagick failed (Check path or permissions)', NULL, '2025-12-01 16:50:29'),
(108, 'ESP32CAM_01', 'C:/AppServ/www/uploads\\meter_ESP32CAM_01_20251201_101251.jpg', 'GE\nZENNER\nl2600000\nSANWA\n70', 100000000.00, '2025-12-01 17:12:54'),
(109, 'ESP32CAM_01', 'C:/AppServ/www/uploads\\meter_ESP32CAM_01_20251201_101738.jpg', '7\nZRI00\n0102\nZENNER\n0000097 m\n40 mm MTKD\n03 16 min\nR10CH R40V\nSANWA\nASAH-THAI ALL', 100000000.00, '2025-12-01 17:17:40');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `meter_readings`
--
ALTER TABLE `meter_readings`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `meter_readings`
--
ALTER TABLE `meter_readings`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=110;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
