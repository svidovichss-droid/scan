#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "gost_standards.h"
#include "datamatrix_scanner.h"

namespace report {

// Структура отчета
struct ReportEntry {
    // Временная метка
    std::string timestamp;
    
    // Результат сканирования
    datamatrix::ScanResult scan_result;
    
    // Изображение (опционально)
    cv::Mat image;
    
    // Путь к сохраненному изображению
    std::string image_path;
    
    // Статус качества (PASS/FAIL)
    std::string quality_status;
};

// Конфигурация генератора отчетов
struct ReportConfig {
    // Путь для сохранения отчетов
    std::string output_path = "./reports/";
    
    // Формат отчета (JSON, CSV, XML, HTML)
    enum Format {
        JSON = 0,
        CSV = 1,
        XML = 2,
        HTML = 3
    };
    
    Format format = JSON;
    
    // Сохранять изображения
    bool save_images = true;
    
    // Максимальное количество записей в одном файле
    int max_entries_per_file = 1000;
    
    // Автоматически создавать новый файл
    bool auto_rotate_files = true;
    
    // Имя файла отчета
    std::string filename_pattern = "datamatrix_report_%Y%m%d_%H%M%S";
};

// Генератор отчетов
class ReportGenerator {
public:
    ReportGenerator();
    ~ReportGenerator();
    
    // Инициализация
    bool initialize(const ReportConfig& config);
    
    // Добавление записи в отчет
    bool addEntry(const ReportEntry& entry);
    
    // Сохранение отчета
    bool saveReport();
    
    // Экспорт в различные форматы
    bool exportToJSON(const std::string& filename);
    bool exportToCSV(const std::string& filename);
    bool exportToXML(const std::string& filename);
    bool exportToHTML(const std::string& filename);
    
    // Получение статистики по отчету
    struct ReportStatistics {
        int total_entries = 0;
        int passed_count = 0;
        int failed_count = 0;
        double average_quality = 0.0;
        double min_quality = 100.0;
        double max_quality = 0.0;
        std::string first_timestamp;
        std::string last_timestamp;
    };
    
    ReportStatistics getStatistics() const;
    
    // Очистка отчета
    void clear();
    
    // Создание сводного отчета за период
    bool generateSummaryReport(const std::string& start_date, 
                               const std::string& end_date,
                               const std::string& output_file);
    
private:
    ReportConfig config_;
    std::vector<ReportEntry> entries_;
    bool initialized_ = false;
    
    // Внутренние методы
    std::string generateTimestamp();
    std::string generateFilename();
    bool ensureDirectoryExists(const std::string& path);
    
    // Методы для разных форматов
    std::string formatJSON(const ReportEntry& entry);
    std::string formatCSV(const ReportEntry& entry);
    std::string formatXML(const ReportEntry& entry);
    std::string formatHTML(const std::vector<ReportEntry>& entries);
    
    // Сохранение изображения
    bool saveImage(const cv::Mat& image, const std::string& filename);
};

} // namespace report

#endif // REPORT_GENERATOR_H
