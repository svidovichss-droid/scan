#include "datamatrix_scanner.h"
#include "quality_analyzer.h"
#include <chrono>
#include <iostream>
#include <filesystem>
// Примечание: ZBar удален из vcpkg. Для декодирования DataMatrix используем:
// 1. Встроенные алгоритмы OpenCV (WeChat QRCode detector поддерживает DataMatrix в новых версиях)
// 2. Проприетарные SDK промышленных камер (Basler, Cognex, Keyence)
// 3. Отдельную библиотеку libdmtx для декодирования

namespace datamatrix {

DataMatrixScanner::DataMatrixScanner() {
    quality_analyzer_ = gost::QualityAnalyzerImpl();
}

DataMatrixScanner::~DataMatrixScanner() {
    stopConveyorScanning();
}

bool DataMatrixScanner::initialize(const ScannerConfig& config) {
    config_ = config;
    
    // Создание директории для отладочных изображений
    if (config_.debug_mode) {
        std::filesystem::create_directories(config_.debug_output_path);
    }
    
    initialized_ = true;
    return true;
}

cv::Mat DataMatrixScanner::preprocessImage(const cv::Mat& image) {
    if (image.empty()) {
        return cv::Mat();
    }
    
    cv::Mat processed;
    
    // Конвертация в оттенки серого
    if (image.channels() > 1) {
        cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = image.clone();
    }
    
    // Применение адаптивного порога
    processed = applyAdaptiveThreshold(processed);
    
    // Удаление шума
    cv::medianBlur(processed, processed, 3);
    
    return processed;
}

cv::Mat DataMatrixScanner::applyAdaptiveThreshold(const cv::Mat& image) {
    cv::Mat binary;
    cv::adaptiveThreshold(image, binary, 255, 
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                         cv::THRESH_BINARY, 11, 2);
    return binary;
}

std::vector<cv::Rect> DataMatrixScanner::findDataMatrixRegions(const cv::Mat& image) {
    std::vector<cv::Rect> regions;
    
    if (image.empty()) {
        return regions;
    }
    
    cv::Mat processed = preprocessImage(image);
    
    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(processed, contours, hierarchy, 
                    cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Фильтрация контуров по площади
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        if (area >= config_.min_area && area <= config_.max_area) {
            cv::Rect bounding_rect = cv::boundingRect(contour);
            
            // Проверка соотношения сторон (DataMatrix обычно квадратный)
            double aspect_ratio = static_cast<double>(bounding_rect.width) / 
                                 bounding_rect.height;
            
            if (aspect_ratio > 0.7 && aspect_ratio < 1.3) {
                regions.push_back(bounding_rect);
            }
        }
    }
    
    return regions;
}

std::string DataMatrixScanner::decodeDataMatrix(const cv::Mat& image, const cv::Rect& region) {
    if (image.empty() || region.empty()) {
        return "";
    }
    
    // Извлечение области интереса
    cv::Mat roi = image(region);
    
    // Примечание: ZBar удален из vcpkg. 
    // В промышленном решении используйте один из вариантов:
    // 1. SDK камеры (Basler pylon, Cognex VisionPro) - имеют встроенные декодеры
    // 2. Библиотеку libdmtx: https://github.com/dmtx/libdmtx
    // 3. OpenCV WeChat QRCode detector (экспериментальная поддержка DataMatrix)
    
    // Для демонстрации возвращаем заглушку - в реальности здесь будет вызов декодера
    // Пример с libdmtx (требует отдельной установки):
    /*
    dmtx::Decoder decoder;
    std::string result = decoder.decode(roi);
    return result;
    */
    
    // Временная реализация для компиляции
    // В продакшене замените на реальный декодер
    return "DEMO_DATA_" + std::to_string(region.x) + "_" + std::to_string(region.y);
}

ScanResult DataMatrixScanner::scan(const cv::Mat& image) {
    ScanResult result;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (image.empty()) {
        result.success = false;
        result.error_message = "Empty image";
        return result;
    }
    
    // Поиск областей DataMatrix
    std::vector<cv::Rect> regions = findDataMatrixRegions(image);
    
    if (regions.empty()) {
        result.success = false;
        result.error_message = "No DataMatrix found";
        return result;
    }
    
    // Обработка первой найденной области
    cv::Rect best_region = regions[0];
    
    // Декодирование
    std::string data = decodeDataMatrix(image, best_region);
    
    if (data.empty()) {
        result.success = false;
        result.error_message = "Failed to decode DataMatrix";
        return result;
    }
    
    // Анализ качества
    gost::QualityResult quality_result = quality_analyzer_.analyze(image);
    
    // Заполнение результата
    result.data = data;
    result.quality = quality_result;
    result.location = best_region;
    result.success = quality_result.passed;
    
    // Расчет времени сканирования
    auto end_time = std::chrono::high_resolution_clock::now();
    result.scan_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
    
    // Обновление статистики
    updateStatistics(result);
    
    // Сохранение отладочного изображения
    if (config_.debug_mode) {
        std::string debug_filename = config_.debug_output_path + 
                                    "scan_" + std::to_string(stats_.total_scans) + ".png";
        cv::Mat debug_image = image.clone();
        cv::rectangle(debug_image, best_region, cv::Scalar(0, 255, 0), 2);
        cv::imwrite(debug_filename, debug_image);
    }
    
    return result;
}

ScanResult DataMatrixScanner::scanFromCamera(camera::CameraCapture& camera) {
    if (!camera.isInitialized()) {
        ScanResult result;
        result.success = false;
        result.error_message = "Camera not initialized";
        return result;
    }
    
    cv::Mat frame = camera.captureFrame();
    return scan(frame);
}

void DataMatrixScanner::startConveyorScanning(camera::CameraCapture& camera,
                                               std::function<void(const ScanResult&)> callback) {
    if (scanning_active_) {
        return;
    }
    
    scanning_active_ = true;
    
    scanning_thread_ = std::thread([this, &camera, callback]() {
        while (scanning_active_) {
            ScanResult result = scanFromCamera(camera);
            
            if (callback) {
                callback(result);
            }
            
            // Небольшая задержка для предотвращения перегрузки
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}

void DataMatrixScanner::stopConveyorScanning() {
    if (scanning_active_) {
        scanning_active_ = false;
        
        if (scanning_thread_.joinable()) {
            scanning_thread_.join();
        }
    }
}

void DataMatrixScanner::updateStatistics(const ScanResult& result) {
    stats_.total_scans++;
    
    if (result.success) {
        stats_.successful_scans++;
    } else {
        stats_.failed_scans++;
    }
    
    // Обновление среднего качества
    double total_quality = stats_.average_quality * (stats_.total_scans - 1);
    stats_.average_quality = (total_quality + result.quality.overall_score) / 
                            stats_.total_scans;
    
    // Обновление среднего времени сканирования
    double total_time = stats_.average_scan_time * (stats_.total_scans - 1);
    stats_.average_scan_time = (total_time + result.scan_time_ms) / 
                              stats_.total_scans;
}

void DataMatrixScanner::resetStatistics() {
    stats_ = Statistics();
}

} // namespace datamatrix
