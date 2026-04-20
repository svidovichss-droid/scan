#ifndef DATAMATRIX_SCANNER_H
#define DATAMATRIX_SCANNER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include "gost_standards.h"
#include "camera_capture.h"

namespace datamatrix {

// Результат сканирования
struct ScanResult {
    // Декодированные данные
    std::string data;
    
    // Качество сканирования
    gost::QualityResult quality;
    
    // Время сканирования (мс)
    double scan_time_ms = 0.0;
    
    // Координаты DataMatrix на изображении
    cv::Rect location;
    
    // Успешность сканирования
    bool success = false;
    
    // Сообщение об ошибке
    std::string error_message;
};

// Конфигурация сканера
struct ScannerConfig {
    // Порог бинаризации
    int binarization_threshold = 128;
    
    // Минимальная площадь DataMatrix (пиксели)
    int min_area = 1000;
    
    // Максимальная площадь DataMatrix (пиксели)
    int max_area = 100000;
    
    // Параметры качества
    gost::QualityParameters quality_params;
    
    // Путь к конфигурации ZBar
    std::string zbar_config;
    
    // Режим отладки (сохранение промежуточных изображений)
    bool debug_mode = false;
    
    // Путь для сохранения отладочных изображений
    std::string debug_output_path = "./debug/";
};

// Основной класс сканера DataMatrix
class DataMatrixScanner {
public:
    DataMatrixScanner();
    ~DataMatrixScanner();
    
    // Инициализация сканера
    bool initialize(const ScannerConfig& config);
    
    // Сканирование изображения
    ScanResult scan(const cv::Mat& image);
    
    // Сканирование с камеры
    ScanResult scanFromCamera(camera::CameraCapture& camera);
    
    // Непрерывное сканирование с конвейера
    void startConveyorScanning(camera::CameraCapture& camera, 
                               std::function<void(const ScanResult&)> callback);
    
    // Остановка непрерывного сканирования
    void stopConveyorScanning();
    
    // Предобработка изображения
    cv::Mat preprocessImage(const cv::Mat& image);
    
    // Поиск области DataMatrix
    std::vector<cv::Rect> findDataMatrixRegions(const cv::Mat& image);
    
    // Декодирование DataMatrix
    std::string decodeDataMatrix(const cv::Mat& image, const cv::Rect& region);
    
    // Получение статистики
    struct Statistics {
        int total_scans = 0;
        int successful_scans = 0;
        int failed_scans = 0;
        double average_quality = 0.0;
        double average_scan_time = 0.0;
    };
    
    Statistics getStatistics() const { return stats_; }
    
    // Сброс статистики
    void resetStatistics();
    
private:
    ScannerConfig config_;
    bool initialized_ = false;
    Statistics stats_;
    
    // Для непрерывного сканирования
    std::thread scanning_thread_;
    std::atomic<bool> scanning_active_{false};
    
    // Внутренние методы
    void updateStatistics(const ScanResult& result);
    cv::Mat applyAdaptiveThreshold(const cv::Mat& image);
    cv::Mat correctPerspective(const cv::Mat& image, const std::vector<cv::Point2f>& corners);
    
    // Анализ качества
    gost::QualityAnalyzer quality_analyzer_;
};

} // namespace datamatrix

#endif // DATAMATRIX_SCANNER_H
