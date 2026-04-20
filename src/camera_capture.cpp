#include "camera_capture.h"
#include <iostream>

namespace camera {

CameraCapture::CameraCapture() {}

CameraCapture::~CameraCapture() {
    stopContinuousCapture();
    if (capture_.isOpened()) {
        capture_.release();
    }
}

bool CameraCapture::initialize(const CameraConfig& config) {
    config_ = config;
    
    // Инициализация в зависимости от типа камеры
    switch (config.camera_type) {
        case 0: // USB камера
            return initializeUSBCamera();
        case 1: // GigE камера
            return initializeGigECamera("");
        case 2: // Camera Link
            return initializeCameraLink();
        default:
            std::cerr << "Unknown camera type: " << config.camera_type << std::endl;
            return false;
    }
}

bool CameraCapture::initializeUSBCamera() {
    // Открытие камеры
    capture_.open(config_.camera_id, cv::CAP_ANY);
    
    if (!capture_.isOpened()) {
        std::cerr << "Failed to open camera: " << config_.camera_id << std::endl;
        return false;
    }
    
    // Настройка разрешения
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
    capture_.set(cv::CAP_PROP_FPS, config_.fps);
    
    // Настройка экспозиции (если поддерживается)
    if (config_.exposure_time > 0) {
        capture_.set(cv::CAP_PROP_EXPOSURE, config_.exposure_time / 1000);
    }
    
    // Настройка усиления (если поддерживается)
    if (config_.gain > 1.0) {
        capture_.set(cv::CAP_PROP_GAIN, config_.gain);
    }
    
    initialized_ = true;
    std::cout << "USB Camera initialized successfully" << std::endl;
    return true;
}

bool CameraCapture::initializeGigECamera(const std::string& ip_address) {
    // Для промышленных GigE камер обычно требуется специфичная библиотека
    // Например: Basler Pylon, FLIR Spinnaker, или OpenCV с соответствующим бэкендом
    
    std::string connection_string;
    
    if (!ip_address.empty()) {
        connection_string = "gvsp://" + ip_address;
    } else {
        connection_string = "gvsp://";
    }
    
    // Попытка открытия через OpenCV (может потребовать специальную сборку)
    capture_.open(connection_string, cv::CAP_ANY);
    
    if (!capture_.isOpened()) {
        std::cerr << "Failed to connect to GigE camera" << std::endl;
        std::cerr << "Note: GigE cameras may require manufacturer SDK" << std::endl;
        return false;
    }
    
    // Настройка параметров
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
    capture_.set(cv::CAP_PROP_FPS, config_.fps);
    
    initialized_ = true;
    std::cout << "GigE Camera initialized successfully" << std::endl;
    return true;
}

bool CameraCapture::initializeCameraLink() {
    // Camera Link требует специфичного оборудования и драйверов
    // Обычно используется через frame grabber с собственным SDK
    
    std::cerr << "Camera Link initialization requires manufacturer SDK" << std::endl;
    std::cerr << "Please integrate with your frame grabber SDK (e.g., National Instruments, Matrox)" << std::endl;
    
    // Заглушка для демонстрации
    capture_.open(0, cv::CAP_ANY);
    initialized_ = capture_.isOpened();
    
    return initialized_;
}

cv::Mat CameraCapture::captureFrame() {
    if (!initialized_ || !capture_.isOpened()) {
        return cv::Mat();
    }
    
    cv::Mat frame;
    capture_ >> frame;
    
    if (frame.empty()) {
        std::cerr << "Captured empty frame" << std::endl;
    }
    
    return frame;
}

void CameraCapture::startContinuousCapture() {
    if (running_) {
        return;
    }
    
    running_ = true;
    capture_thread_ = std::thread(&CameraCapture::captureLoop, this);
}

void CameraCapture::stopContinuousCapture() {
    if (running_) {
        running_ = false;
        
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
    }
}

void CameraCapture::captureLoop() {
    while (running_) {
        cv::Mat frame = captureFrame();
        
        if (!frame.empty()) {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            latest_frame_ = frame;
        }
        
        // Небольшая задержка для предотвращения перегрузки CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

cv::Mat CameraCapture::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    return latest_frame_.clone();
}

bool CameraCapture::setExposure(int exposure_us) {
    if (!initialized_) {
        return false;
    }
    
    config_.exposure_time = exposure_us;
    
    // Конвертация мкс в единицы OpenCV (зависит от камеры)
    double exposure_value = exposure_us / 1000.0;
    
    return capture_.set(cv::CAP_PROP_EXPOSURE, exposure_value);
}

bool CameraCapture::setGain(double gain) {
    if (!initialized_) {
        return false;
    }
    
    config_.gain = gain;
    return capture_.set(cv::CAP_PROP_GAIN, gain);
}

bool CameraCapture::setFPS(int fps) {
    if (!initialized_) {
        return false;
    }
    
    config_.fps = fps;
    return capture_.set(cv::CAP_PROP_FPS, fps);
}

std::string CameraCapture::getCameraInfo() {
    if (!initialized_) {
        return "Camera not initialized";
    }
    
    std::string info = "Camera Information:\n";
    info += "  Type: " + std::to_string(config_.camera_type) + "\n";
    info += "  Resolution: " + std::to_string(config_.width) + "x" + 
            std::to_string(config_.height) + "\n";
    info += "  FPS: " + std::to_string(capture_.get(cv::CAP_PROP_FPS)) + "\n";
    info += "  Exposure: " + std::to_string(capture_.get(cv::CAP_PROP_EXPOSURE)) + "\n";
    info += "  Gain: " + std::to_string(capture_.get(cv::CAP_PROP_GAIN)) + "\n";
    
    return info;
}

bool CameraCapture::connectIndustrialCamera(const std::string& ip_address) {
    config_.camera_type = 1; // GigE
    return initializeGigECamera(ip_address);
}

} // namespace camera
