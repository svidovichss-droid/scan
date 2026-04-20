#ifndef GOST_STANDARDS_H
#define GOST_STANDARDS_H

#include <string>
#include <vector>
#include <cmath>

namespace gost {

// Параметры качества по ГОСТ Р 57302-2016 и ISO/IEC 16022
struct QualityParameters {
    // Минимальный размер модуля (мм)
    double min_module_size = 0.254;
    
    // Максимальный размер модуля (мм)
    double max_module_size = 1.016;
    
    // Минимальная контрастность (0-100%)
    double min_contrast = 20.0;
    
    // Минимальное значение дефектности (0-100%)
    double min_defect_score = 70.0;
    
    // Допустимое отклонение угла поворота (градусы)
    double max_rotation_deviation = 5.0;
    
    // Минимальная ширина тихой зоны (в модулях)
    double min_quiet_zone = 1.0;
};

// Результаты анализа качества
struct QualityResult {
    // Общая оценка качества (0-100)
    double overall_score = 0.0;
    
    // Оценка контрастности
    double contrast_score = 0.0;
    
    // Оценка дефектности
    double defect_score = 0.0;
    
    // Оценка геометрии
    double geometry_score = 0.0;
    
    // Оценка модуляции
    double modulation_score = 0.0;
    
    // Размер модуля (мм)
    double module_size = 0.0;
    
    // Угол поворота (градусы)
    double rotation_angle = 0.0;
    
    // Ширина тихой зоны (в модулях)
    double quiet_zone = 0.0;
    
    // Статус прохождения проверки
    bool passed = false;
    
    // Сообщение об ошибках
    std::string error_message;
    
    // Декодированные данные
    std::string decoded_data;
};

// Класс для анализа качества по стандартам
class QualityAnalyzer {
public:
    QualityAnalyzer(const QualityParameters& params = QualityParameters());
    
    // Анализ качества изображения
    QualityResult analyze(const cv::Mat& image);
    
    // Проверка соответствия стандарту
    bool checkCompliance(const QualityResult& result);
    
    // Расчет контрастности
    double calculateContrast(const cv::Mat& image);
    
    // Расчет дефектности
    double calculateDefect(const cv::Mat& image, const cv::Rect& datamatrix_region);
    
    // Расчет геометрии
    double calculateGeometry(const std::vector<cv::Point2f>& corners);
    
    // Расчет модуляции
    double calculateModulation(const cv::Mat& image, const cv::Rect& datamatrix_region);
    
private:
    QualityParameters params_;
    
    // Поиск области DataMatrix
    cv::Rect findDataMatrixRegion(const cv::Mat& image);
    
    // Определение углов DataMatrix
    std::vector<cv::Point2f> detectCorners(const cv::Mat& image, const cv::Rect& region);
    
    // Измерение размера модуля
    double measureModuleSize(const cv::Mat& image, const cv::Rect& region);
    
    // Измерение тихой зоны
    double measureQuietZone(const cv::Mat& image, const cv::Rect& region);
};

} // namespace gost

#endif // GOST_STANDARDS_H
