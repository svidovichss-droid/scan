#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace camera {

// Конфигурация камеры
struct CameraConfig {
    // ID камеры (0 для первой камеры)
    int camera_id = 0;
    
    // Разрешение по ширине
    int width = 1920;
    
    // Разрешение по высоте
    int height = 1080;
    
    // Частота кадров (FPS)
    int fps = 30;
    
    // Время экспозиции (мкс)
    int exposure_time = 5000;
    
    // Усиление (gain)
    double gain = 1.0;
    
    // Путь к конфигурации камеры (для промышленных камер)
    std::string config_file;
    
    // Тип камеры (0 - USB, 1 - GigE, 2 - Camera Link)
    int camera_type = 0;
};

// Класс для захвата изображения с промышленной камеры
class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();
    
    // Инициализация камеры
    bool initialize(const CameraConfig& config);
    
    // Захват одного кадра
    cv::Mat captureFrame();
    
    // Запуск непрерывного захвата
    void startContinuousCapture();
    
    // Остановка непрерывного захвата
    void stopContinuousCapture();
    
    // Проверка состояния камеры
    bool isInitialized() const { return initialized_; }
    
    // Получение последнего кадра
    cv::Mat getLatestFrame();
    
    // Настройка параметров камеры
    bool setExposure(int exposure_us);
    bool setGain(double gain);
    bool setFPS(int fps);
    
    // Получение информации о камере
    std::string getCameraInfo();
    
    // Подключение к промышленной камере (GigE, Camera Link)
    bool connectIndustrialCamera(const std::string& ip_address = "");
    
private:
    cv::VideoCapture capture_;
    CameraConfig config_;
    bool initialized_ = false;
    
    // Для непрерывного захвата
    std::thread capture_thread_;
    std::atomic<bool> running_{false};
    std::mutex frame_mutex_;
    cv::Mat latest_frame_;
    
    // Поток захвата
    void captureLoop();
    
    // Специфичные функции для промышленных камер
    bool initializeUSBCamera();
    bool initializeGigECamera(const std::string& ip_address);
    bool initializeCameraLink();
};

} // namespace camera

#endif // CAMERA_CAPTURE_H
