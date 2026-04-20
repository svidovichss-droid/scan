#include <iostream>
#include <opencv2/opencv.hpp>
#include "datamatrix_scanner.h"
#include "camera_capture.h"
#include "report_generator.h"
#include <csignal>
#include <atomic>

// Глобальный флаг для остановки программы
std::atomic<bool> g_running(true);

// Обработчик сигнала завершения
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Stopping..." << std::endl;
    g_running = false;
}

// Конфигурация приложения
struct AppConfig {
    // Режим работы (0 - тестовый, 1 - конвейер)
    int mode = 0;
    
    // ID камеры
    int camera_id = 0;
    
    // Тип камеры (0 - USB, 1 - GigE, 2 - Camera Link)
    int camera_type = 0;
    
    // IP адрес для GigE камеры
    std::string camera_ip;
    
    // Путь для сохранения отчетов
    std::string report_path = "./reports/";
    
    // Режим отладки
    bool debug_mode = false;
    
    // Разрешение камеры
    int width = 1920;
    int height = 1080;
    
    // Частота кадров
    int fps = 30;
};

// Парсинг аргументов командной строки
AppConfig parseArgs(int argc, char* argv[]) {
    AppConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--mode" && i + 1 < argc) {
            config.mode = std::stoi(argv[++i]);
        }
        else if (arg == "--camera" && i + 1 < argc) {
            config.camera_id = std::stoi(argv[++i]);
        }
        else if (arg == "--camera-type" && i + 1 < argc) {
            config.camera_type = std::stoi(argv[++i]);
        }
        else if (arg == "--camera-ip" && i + 1 < argc) {
            config.camera_ip = argv[++i];
        }
        else if (arg == "--report-path" && i + 1 < argc) {
            config.report_path = argv[++i];
        }
        else if (arg == "--debug") {
            config.debug_mode = true;
        }
        else if (arg == "--width" && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        }
        else if (arg == "--height" && i + 1 < argc) {
            config.height = std::stoi(argv[++i]);
        }
        else if (arg == "--fps" && i + 1 < argc) {
            config.fps = std::stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "DataMatrix Quality Scanner\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --mode <0|1>       Mode: 0=test, 1=conveyor (default: 0)\n";
            std::cout << "  --camera <id>      Camera ID (default: 0)\n";
            std::cout << "  --camera-type <t>  Camera type: 0=USB, 1=GigE, 2=CameraLink (default: 0)\n";
            std::cout << "  --camera-ip <ip>   IP address for GigE camera\n";
            std::cout << "  --report-path <p>  Path for reports (default: ./reports/)\n";
            std::cout << "  --debug            Enable debug mode\n";
            std::cout << "  --width <w>        Camera width (default: 1920)\n";
            std::cout << "  --height <h>       Camera height (default: 1080)\n";
            std::cout << "  --fps <f>          Camera FPS (default: 30)\n";
            std::cout << "  --help, -h         Show this help\n";
            exit(0);
        }
    }
    
    return config;
}

// Тестовый режим
void runTestMode(const AppConfig& config) {
    std::cout << "Running in TEST MODE\n";
    
    // Инициализация камеры
    camera::CameraCapture cam;
    camera::CameraConfig cam_config;
    cam_config.camera_id = config.camera_id;
    cam_config.camera_type = config.camera_type;
    cam_config.width = config.width;
    cam_config.height = config.height;
    cam_config.fps = config.fps;
    
    if (!cam.initialize(cam_config)) {
        std::cerr << "Failed to initialize camera. Using test image.\n";
    }
    
    // Инициализация сканера
    datamatrix::DataMatrixScanner scanner;
    datamatrix::ScannerConfig scan_config;
    scan_config.debug_mode = config.debug_mode;
    
    if (!scanner.initialize(scan_config)) {
        std::cerr << "Failed to initialize scanner\n";
        return;
    }
    
    // Инициализация генератора отчетов
    report::ReportGenerator report_gen;
    report::ReportConfig report_config;
    report_config.output_path = config.report_path;
    report_config.format = report::ReportConfig::JSON;
    
    if (!report_gen.initialize(report_config)) {
        std::cerr << "Failed to initialize report generator\n";
        return;
    }
    
    std::cout << "Press 'q' to quit\n";
    
    // Создание окна для отображения
    cv::namedWindow("DataMatrix Scanner", cv::WINDOW_NORMAL);
    
    while (g_running) {
        // Захват кадра
        cv::Mat frame;
        
        if (cam.isInitialized()) {
            frame = cam.captureFrame();
        }
        
        // Если камера не работает или кадр пустой, создаем тестовое изображение
        if (frame.empty()) {
            frame = cv::Mat(480, 640, CV_8UC3, cv::Scalar(255, 255, 255));
            
            // Рисуем имитацию DataMatrix
            cv::Rect dm_rect(200, 150, 240, 240);
            cv::rectangle(frame, dm_rect, cv::Scalar(0, 0, 0), -1);
            
            // Добавим шум для реалистичности
            cv::randn(frame, cv::Scalar(0, 0, 0), cv::Scalar(10, 10, 10));
        }
        
        // Сканирование
        datamatrix::ScanResult result = scanner.scan(frame);
        
        // Отображение результата
        cv::Mat display_frame = frame.clone();
        
        if (result.success) {
            // Рисуем рамку вокруг DataMatrix
            cv::rectangle(display_frame, result.location, cv::Scalar(0, 255, 0), 2);
            
            // Отображаем информацию
            std::string info = "Data: " + result.data;
            cv::putText(display_frame, info, 
                       cv::Point(10, 30), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                       cv::Scalar(0, 255, 0), 2);
            
            info = "Quality: " + std::to_string(static_cast<int>(result.quality.overall_score)) + "%";
            cv::putText(display_frame, info, 
                       cv::Point(10, 60), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                       cv::Scalar(0, 255, 0), 2);
            
            info = "Status: PASS";
            cv::putText(display_frame, info, 
                       cv::Point(10, 90), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                       cv::Scalar(0, 255, 0), 2);
            
            // Добавление в отчет
            report::ReportEntry entry;
            entry.timestamp = std::to_string(std::time(nullptr));
            entry.scan_result = result;
            entry.quality_status = "PASS";
            report_gen.addEntry(entry);
            
        } else {
            // Отображаем сообщение об ошибке
            std::string error = "No DataMatrix found";
            if (!result.error_message.empty()) {
                error = result.error_message;
            }
            
            cv::putText(display_frame, error, 
                       cv::Point(10, 30), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                       cv::Scalar(0, 0, 255), 2);
        }
        
        // Отображение статистики
        auto stats = scanner.getStatistics();
        std::string stat_text = "Total: " + std::to_string(stats.total_scans) +
                               " | Success: " + std::to_string(stats.successful_scans) +
                               " | Avg Quality: " + std::to_string(static_cast<int>(stats.average_quality)) + "%";
        
        cv::putText(display_frame, stat_text, 
                   cv::Point(10, display_frame.rows - 10), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                   cv::Scalar(0, 0, 0), 1);
        
        // Показываем кадр
        cv::imshow("DataMatrix Scanner", display_frame);
        
        // Обработка клавиш
        char key = cv::waitKey(1);
        if (key == 'q' || key == 'Q') {
            break;
        }
    }
    
    // Сохранение отчета
    report_gen.saveReport();
    
    // Вывод итоговой статистики
    auto stats = scanner.getStatistics();
    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "Total scans: " << stats.total_scans << std::endl;
    std::cout << "Successful: " << stats.successful_scans << std::endl;
    std::cout << "Failed: " << stats.failed_scans << std::endl;
    std::cout << "Average quality: " << stats.average_quality << "%" << std::endl;
    std::cout << "Average scan time: " << stats.average_scan_time << " ms" << std::endl;
}

// Режим конвейера
void runConveyorMode(const AppConfig& config) {
    std::cout << "Running in CONVEYOR MODE\n";
    
    // Инициализация камеры
    camera::CameraCapture cam;
    camera::CameraConfig cam_config;
    cam_config.camera_id = config.camera_id;
    cam_config.camera_type = config.camera_type;
    cam_config.width = config.width;
    cam_config.height = config.height;
    cam_config.fps = config.fps;
    
    if (!cam.initialize(cam_config)) {
        std::cerr << "Failed to initialize camera\n";
        return;
    }
    
    // Инициализация сканера
    datamatrix::DataMatrixScanner scanner;
    datamatrix::ScannerConfig scan_config;
    scan_config.debug_mode = config.debug_mode;
    
    if (!scanner.initialize(scan_config)) {
        std::cerr << "Failed to initialize scanner\n";
        return;
    }
    
    // Инициализация генератора отчетов
    report::ReportGenerator report_gen;
    report::ReportConfig report_config;
    report_config.output_path = config.report_path;
    report_config.format = report::ReportConfig::CSV;
    
    if (!report_gen.initialize(report_config)) {
        std::cerr << "Failed to initialize report generator\n";
        return;
    }
    
    std::cout << "Starting continuous scanning. Press Ctrl+C to stop.\n";
    
    // Callback для обработки результатов
    auto callback = [&report_gen](const datamatrix::ScanResult& result) {
        if (result.success) {
            std::cout << "[PASS] Data: " << result.data 
                     << " | Quality: " << result.quality.overall_score << "%"
                     << " | Time: " << result.scan_time_ms << " ms" << std::endl;
            
            // Добавление в отчет
            report::ReportEntry entry;
            entry.timestamp = std::to_string(std::time(nullptr));
            entry.scan_result = result;
            entry.quality_status = "PASS";
            report_gen.addEntry(entry);
            
        } else {
            std::cout << "[FAIL] " << result.error_message << std::endl;
        }
    };
    
    // Запуск непрерывного сканирования
    scanner.startConveyorScanning(cam, callback);
    
    // Ожидание сигнала завершения
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Остановка сканирования
    scanner.stopConveyorScanning();
    
    // Сохранение отчета
    report_gen.saveReport();
    
    // Вывод итоговой статистики
    auto stats = scanner.getStatistics();
    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "Total scans: " << stats.total_scans << std::endl;
    std::cout << "Successful: " << stats.successful_scans << std::endl;
    std::cout << "Failed: " << stats.failed_scans << std::endl;
    std::cout << "Average quality: " << stats.average_quality << "%" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "===========================================\n";
    std::cout << "  DataMatrix Quality Scanner\n";
    std::cout << "  ГОСТ Р 57302-2016 / ISO/IEC 16022\n";
    std::cout << "===========================================\n\n";
    
    // Установка обработчика сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Парсинг аргументов
    AppConfig config = parseArgs(argc, argv);
    
    // Запуск соответствующего режима
    if (config.mode == 0) {
        runTestMode(config);
    } else {
        runConveyorMode(config);
    }
    
    std::cout << "\nApplication terminated gracefully.\n";
    
    return 0;
}
