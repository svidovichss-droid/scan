#ifndef QUALITY_ANALYZER_H
#define QUALITY_ANALYZER_H

#include "gost_standards.h"
#include <opencv2/opencv.hpp>

namespace gost {

// Реализация анализатора качества
class QualityAnalyzerImpl : public QualityAnalyzer {
public:
    QualityAnalyzerImpl(const QualityParameters& params = QualityParameters());
    
    QualityResult analyze(const cv::Mat& image) override;
    bool checkCompliance(const QualityResult& result) override;
    double calculateContrast(const cv::Mat& image) override;
    double calculateDefect(const cv::Mat& image, const cv::Rect& datamatrix_region) override;
    double calculateGeometry(const std::vector<cv::Point2f>& corners) override;
    double calculateModulation(const cv::Mat& image, const cv::Rect& datamatrix_region) override;
    
private:
    cv::Rect findDataMatrixRegion(const cv::Mat& image) override;
    std::vector<cv::Point2f> detectCorners(const cv::Mat& image, const cv::Rect& region) override;
    double measureModuleSize(const cv::Mat& image, const cv::Rect& region) override;
    double measureQuietZone(const cv::Mat& image, const cv::Rect& region) override;
    
    // Вспомогательные методы
    double calculateSNR(const cv::Mat& image);
    double calculateEdgeSharpness(const cv::Mat& image);
    int countDecodableModules(const cv::Mat& image, const cv::Rect& region);
};

} // namespace gost

#endif // QUALITY_ANALYZER_H
