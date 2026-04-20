#include "quality_analyzer.h"
#include <iostream>
#include <algorithm>

namespace gost {

QualityAnalyzerImpl::QualityAnalyzerImpl(const QualityParameters& params) 
    : params_(params) {}

QualityResult QualityAnalyzerImpl::analyze(const cv::Mat& image) {
    QualityResult result;
    
    if (image.empty()) {
        result.error_message = "Empty image";
        return result;
    }
    
    // Поиск области DataMatrix
    cv::Rect datamatrix_region = findDataMatrixRegion(image);
    
    if (datamatrix_region.empty()) {
        result.error_message = "DataMatrix not found";
        return result;
    }
    
    // Определение углов
    std::vector<cv::Point2f> corners = detectCorners(image, datamatrix_region);
    
    // Измерение размера модуля
    result.module_size = measureModuleSize(image, datamatrix_region);
    
    // Измерение тихой зоны
    result.quiet_zone = measureQuietZone(image, datamatrix_region);
    
    // Расчет угла поворота
    if (corners.size() >= 4) {
        double dx = corners[1].x - corners[0].x;
        double dy = corners[1].y - corners[0].y;
        result.rotation_angle = std::atan2(dy, dx) * 180.0 / CV_PI;
    }
    
    // Расчет параметров качества
    result.contrast_score = calculateContrast(image);
    result.defect_score = calculateDefect(image, datamatrix_region);
    result.geometry_score = calculateGeometry(corners);
    result.modulation_score = calculateModulation(image, datamatrix_region);
    
    // Общая оценка качества (взвешенное среднее)
    result.overall_score = (
        result.contrast_score * 0.25 +
        result.defect_score * 0.30 +
        result.geometry_score * 0.25 +
        result.modulation_score * 0.20
    );
    
    // Проверка соответствия стандарту
    result.passed = checkCompliance(result);
    
    if (!result.passed) {
        // Формирование сообщения об ошибках
        if (result.contrast_score < params_.min_contrast) {
            result.error_message += "Low contrast. ";
        }
        if (result.defect_score < params_.min_defect_score) {
            result.error_message += "High defect rate. ";
        }
        if (std::abs(result.rotation_angle) > params_.max_rotation_deviation) {
            result.error_message += "Excessive rotation. ";
        }
        if (result.quiet_zone < params_.min_quiet_zone) {
            result.error_message += "Insufficient quiet zone. ";
        }
    }
    
    return result;
}

bool QualityAnalyzerImpl::checkCompliance(const QualityResult& result) {
    // Проверка по параметрам ГОСТ Р 57302-2016 и ISO/IEC 16022
    
    bool passed = true;
    
    // Проверка контрастности
    if (result.contrast_score < params_.min_contrast) {
        passed = false;
    }
    
    // Проверка дефектности
    if (result.defect_score < params_.min_defect_score) {
        passed = false;
    }
    
    // Проверка геометрии
    if (result.geometry_score < params_.min_defect_score) {
        passed = false;
    }
    
    // Проверка угла поворота
    if (std::abs(result.rotation_angle) > params_.max_rotation_deviation) {
        passed = false;
    }
    
    // Проверка тихой зоны
    if (result.quiet_zone < params_.min_quiet_zone) {
        passed = false;
    }
    
    // Проверка размера модуля
    if (result.module_size < params_.min_module_size || 
        result.module_size > params_.max_module_size) {
        passed = false;
    }
    
    return passed;
}

double QualityAnalyzerImpl::calculateContrast(const cv::Mat& image) {
    if (image.empty()) {
        return 0.0;
    }
    
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    // Расчет гистограммы
    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* ranges[] = {range};
    
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, ranges);
    
    // Поиск пиков для темных и светлых областей
    double max_val = 0;
    cv::minMaxLoc(hist, nullptr, &max_val);
    
    // Расчет контраста как разница между средними значениями темных и светлых пикселей
    double dark_sum = 0, light_sum = 0;
    int dark_count = 0, light_count = 0;
    
    for (int i = 0; i < histSize / 2; i++) {
        dark_sum += i * hist.at<float>(i);
        dark_count += hist.at<float>(i);
    }
    
    for (int i = histSize / 2; i < histSize; i++) {
        light_sum += i * hist.at<float>(i);
        light_count += hist.at<float>(i);
    }
    
    double dark_mean = dark_count > 0 ? dark_sum / dark_count : 0;
    double light_mean = light_count > 0 ? light_sum / light_count : 0;
    
    double contrast = (light_mean - dark_mean) / 255.0 * 100.0;
    
    return std::min(100.0, std::max(0.0, contrast));
}

double QualityAnalyzerImpl::calculateDefect(const cv::Mat& image, 
                                            const cv::Rect& datamatrix_region) {
    if (image.empty() || datamatrix_region.empty()) {
        return 0.0;
    }
    
    cv::Mat roi = image(datamatrix_region);
    cv::Mat gray;
    
    if (roi.channels() > 1) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }
    
    // Бинаризация
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    
    // Подсчет количества дефектов (неправильных модулей)
    int total_modules = 0;
    int correct_modules = 0;
    
    // Оценка размера модуля
    int module_size = std::min(roi.rows, roi.cols) / 20; // Примерная оценка
    
    for (int y = 0; y < roi.rows - module_size; y += module_size) {
        for (int x = 0; x < roi.cols - module_size; x += module_size) {
            cv::Rect module_rect(x, y, module_size, module_size);
            cv::Mat module = binary(module_rect);
            
            // Подсчет белых и черных пикселей в модуле
            double white_ratio = cv::countNonZero(module) / 
                                static_cast<double>(module.total());
            
            total_modules++;
            
            // Модуль считается корректным, если он преимущественно белый или черный
            if (white_ratio > 0.7 || white_ratio < 0.3) {
                correct_modules++;
            }
        }
    }
    
    double defect_rate = total_modules > 0 ? 
                        (correct_modules * 100.0 / total_modules) : 0.0;
    
    return std::min(100.0, std::max(0.0, defect_rate));
}

double QualityAnalyzerImpl::calculateGeometry(const std::vector<cv::Point2f>& corners) {
    if (corners.size() < 4) {
        return 0.0;
    }
    
    // Проверка квадратности (соотношение сторон и углы)
    std::vector<double> side_lengths;
    
    for (size_t i = 0; i < 4; i++) {
        cv::Point2f p1 = corners[i];
        cv::Point2f p2 = corners[(i + 1) % 4];
        double length = cv::norm(p1 - p2);
        side_lengths.push_back(length);
    }
    
    // Расчет соотношения максимальной и минимальной стороны
    double max_side = *std::max_element(side_lengths.begin(), side_lengths.end());
    double min_side = *std::min_element(side_lengths.begin(), side_lengths.end());
    
    double aspect_ratio_score = (min_side / max_side) * 100.0;
    
    // Проверка углов (должны быть близки к 90 градусам)
    double angle_score = 100.0;
    
    for (size_t i = 0; i < 4; i++) {
        cv::Point2f p1 = corners[i];
        cv::Point2f p2 = corners[(i + 1) % 4];
        cv::Point2f p3 = corners[(i + 2) % 4];
        
        double angle = std::acos(
            ((p2.x - p1.x) * (p3.x - p2.x) + (p2.y - p1.y) * (p3.y - p2.y)) /
            (cv::norm(p2 - p1) * cv::norm(p3 - p2))
        ) * 180.0 / CV_PI;
        
        double deviation = std::abs(90.0 - angle);
        angle_score -= deviation * 2; // Штраф за отклонение от 90 градусов
    }
    
    angle_score = std::max(0.0, angle_score);
    
    // Итоговая оценка геометрии
    return (aspect_ratio_score * 0.5 + angle_score * 0.5);
}

double QualityAnalyzerImpl::calculateModulation(const cv::Mat& image, 
                                                const cv::Rect& datamatrix_region) {
    if (image.empty() || datamatrix_region.empty()) {
        return 0.0;
    }
    
    cv::Mat roi = image(datamatrix_region);
    cv::Mat gray;
    
    if (roi.channels() > 1) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }
    
    // Расчет модуляции через анализ профиля яркости
    cv::Mat profile_x, profile_y;
    
    // Горизонтальный профиль
    cv::reduce(gray, profile_x, 0, cv::REDUCE_AVG);
    
    // Вертикальный профиль
    cv::reduce(gray, profile_y, 1, cv::REDUCE_AVG);
    
    // Расчет модуляции как отношение разницы максимума и минимума к их сумме
    double max_x, min_x, max_y, min_y;
    cv::minMaxLoc(profile_x, &min_x, &max_x);
    cv::minMaxLoc(profile_y, &min_y, &max_y);
    
    double modulation_x = (max_x - min_x) / (max_x + min_x) * 100.0;
    double modulation_y = (max_y - min_y) / (max_y + min_y) * 100.0;
    
    return (modulation_x + modulation_y) / 2.0;
}

cv::Rect QualityAnalyzerImpl::findDataMatrixRegion(const cv::Mat& image) {
    if (image.empty()) {
        return cv::Rect();
    }
    
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    // Бинаризация
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
    
    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, 
                    cv::CHAIN_APPROX_SIMPLE);
    
    // Поиск наиболее подходящего контура
    cv::Rect best_rect;
    double best_score = 0;
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        // Фильтрация по площади
        if (area < 1000 || area > 100000) {
            continue;
        }
        
        cv::Rect rect = cv::boundingRect(contour);
        
        // Проверка соотношения сторон (близко к квадрату)
        double aspect_ratio = static_cast<double>(rect.width) / rect.height;
        
        if (aspect_ratio < 0.7 || aspect_ratio > 1.3) {
            continue;
        }
        
        // Оценка качества (площадь * близость к квадрату)
        double score = area * (1.0 - std::abs(1.0 - aspect_ratio));
        
        if (score > best_score) {
            best_score = score;
            best_rect = rect;
        }
    }
    
    return best_rect;
}

std::vector<cv::Point2f> QualityAnalyzerImpl::detectCorners(const cv::Mat& image, 
                                                            const cv::Rect& region) {
    std::vector<cv::Point2f> corners;
    
    if (image.empty() || region.empty()) {
        return corners;
    }
    
    cv::Mat roi = image(region);
    cv::Mat gray;
    
    if (roi.channels() > 1) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }
    
    // Детектор углов Harris
    cv::Mat dst, dst_norm, dst_norm_scaled;
    dst = cv::Mat::zeros(gray.size(), CV_32FC1);
    
    cv::cornerHarris(gray, dst, 2, 3, 0.04);
    
    cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX);
    cv::convertScaleAbs(dst_norm, dst_norm_scaled);
    
    // Поиск углов
    std::vector<cv::Point2f> all_corners;
    
    for (int j = 0; j < dst_norm.rows; j++) {
        for (int i = 0; i < dst_norm.cols; i++) {
            if (dst_norm.at<float>(j, i) > 200) {
                all_corners.push_back(cv::Point2f(i, j));
            }
        }
    }
    
    // Выбор 4 основных углов (по углам региона)
    if (all_corners.size() >= 4) {
        // Сортировка по расстоянию до углов региона
        std::vector<cv::Point2f> corner_candidates = {
            cv::Point2f(0, 0),
            cv::Point2f(region.width, 0),
            cv::Point2f(region.width, region.height),
            cv::Point2f(0, region.height)
        };
        
        for (const auto& candidate : corner_candidates) {
            double min_dist = std::numeric_limits<double>::max();
            cv::Point2f closest_corner;
            
            for (const auto& corner : all_corners) {
                double dist = cv::norm(candidate - corner);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_corner = corner;
                }
            }
            
            // Добавление смещения региона
            closest_corner += cv::Point2f(region.x, region.y);
            corners.push_back(closest_corner);
        }
    }
    
    return corners;
}

double QualityAnalyzerImpl::measureModuleSize(const cv::Mat& image, 
                                              const cv::Rect& region) {
    if (image.empty() || region.empty()) {
        return 0.0;
    }
    
    // Оценка размера модуля на основе размера региона
    // Типичный DataMatrix имеет размер от 10x10 до 144x144 модулей
    // Используем среднее значение для оценки
    
    int min_dimension = std::min(region.width, region.height);
    
    // Предположим средний размер матрицы 40x40 модулей
    double estimated_module_size_pixels = min_dimension / 40.0;
    
    // Конвертация в миллиметры (предположение: 10 пикселей = 1 мм)
    // В реальном приложении нужно использовать калибровку камеры
    double module_size_mm = estimated_module_size_pixels / 10.0;
    
    return module_size_mm;
}

double QualityAnalyzerImpl::measureQuietZone(const cv::Mat& image, 
                                             const cv::Rect& region) {
    if (image.empty() || region.empty()) {
        return 0.0;
    }
    
    // Анализ области вокруг DataMatrix для оценки тихой зоны
    int margin = 20; // Пикселей для анализа
    
    cv::Rect extended_region(
        std::max(0, region.x - margin),
        std::max(0, region.y - margin),
        std::min(image.cols, region.width + 2 * margin),
        std::min(image.rows, region.height + 2 * margin)
    );
    
    cv::Mat surrounding = image(extended_region);
    cv::Mat gray;
    
    if (surrounding.channels() > 1) {
        cv::cvtColor(surrounding, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = surrounding;
    }
    
    // Расчет средней яркости окружающей области
    double mean_brightness = cv::mean(gray)[0];
    
    // Тихая зона должна быть однородной и светлой
    // Оценка в модулях (предположение: 1 модуль = 10 пикселей)
    double quiet_zone_pixels = margin;
    double quiet_zone_modules = quiet_zone_pixels / 10.0;
    
    return quiet_zone_modules;
}

double QualityAnalyzerImpl::calculateSNR(const cv::Mat& image) {
    // Расчет отношения сигнал/шум
    if (image.empty()) {
        return 0.0;
    }
    
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    
    return mean[0] / (stddev[0] + 1e-6);
}

double QualityAnalyzerImpl::calculateEdgeSharpness(const cv::Mat& image) {
    // Расчет резкости краев
    if (image.empty()) {
        return 0.0;
    }
    
    cv::Mat gray;
    if (image.channels() > 1) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    // Применение оператора Лапласа
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);
    
    // Расчет дисперсии Лапласиана как меры резкости
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);
    
    return stddev[0];
}

int QualityAnalyzerImpl::countDecodableModules(const cv::Mat& image, 
                                               const cv::Rect& region) {
    // Подсчет декодируемых модулей
    if (image.empty() || region.empty()) {
        return 0;
    }
    
    cv::Mat roi = image(region);
    cv::Mat gray;
    
    if (roi.channels() > 1) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }
    
    // Бинаризация
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    
    // Подсчет переходов между черным и белым
    int transitions = 0;
    
    for (int y = 0; y < binary.rows; y++) {
        uchar* row = binary.ptr<uchar>(y);
        for (int x = 1; x < binary.cols; x++) {
            if (row[x] != row[x-1]) {
                transitions++;
            }
        }
    }
    
    return transitions;
}

} // namespace gost
