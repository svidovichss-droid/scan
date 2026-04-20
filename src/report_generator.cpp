#include "report_generator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

namespace report {

ReportGenerator::ReportGenerator() {}

ReportGenerator::~ReportGenerator() {
    saveReport();
}

bool ReportGenerator::initialize(const ReportConfig& config) {
    config_ = config;
    
    // Создание директории для отчетов
    if (!ensureDirectoryExists(config_.output_path)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool ReportGenerator::addEntry(const ReportEntry& entry) {
    if (!initialized_) {
        return false;
    }
    
    entries_.push_back(entry);
    
    // Автоматическое сохранение при достижении лимита
    if (config_.auto_rotate_files && 
        entries_.size() >= static_cast<size_t>(config_.max_entries_per_file)) {
        saveReport();
        clear();
    }
    
    return true;
}

bool ReportGenerator::saveReport() {
    if (!initialized_ || entries_.empty()) {
        return false;
    }
    
    std::string filename = generateFilename();
    
    switch (config_.format) {
        case ReportConfig::JSON:
            return exportToJSON(filename + ".json");
        case ReportConfig::CSV:
            return exportToCSV(filename + ".csv");
        case ReportConfig::XML:
            return exportToXML(filename + ".xml");
        case ReportConfig::HTML:
            return exportToHTML(filename + ".html");
        default:
            return exportToJSON(filename + ".json");
    }
}

bool ReportGenerator::exportToJSON(const std::string& filename) {
    std::ofstream file(config_.output_path + filename);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    file << "  \"report_info\": {\n";
    file << "    \"generated_at\": \"" << generateTimestamp() << "\",\n";
    file << "    \"total_entries\": " << entries_.size() << "\n";
    file << "  },\n";
    file << "  \"entries\": [\n";
    
    for (size_t i = 0; i < entries_.size(); i++) {
        const auto& entry = entries_[i];
        
        file << "    {\n";
        file << "      \"timestamp\": \"" << entry.timestamp << "\",\n";
        file << "      \"data\": \"" << entry.scan_result.data << "\",\n";
        file << "      \"quality_score\": " << entry.scan_result.quality.overall_score << ",\n";
        file << "      \"passed\": " << (entry.scan_result.success ? "true" : "false") << ",\n";
        file << "      \"scan_time_ms\": " << entry.scan_result.scan_time_ms << "\n";
        file << "    }";
        
        if (i < entries_.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    return true;
}

bool ReportGenerator::exportToCSV(const std::string& filename) {
    std::ofstream file(config_.output_path + filename);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Заголовок
    file << "Timestamp,Data,Quality Score,Passed,Scan Time (ms),Error Message\n";
    
    for (const auto& entry : entries_) {
        file << entry.timestamp << ","
             << "\"" << entry.scan_result.data << "\","
             << entry.scan_result.quality.overall_score << ","
             << (entry.scan_result.success ? "PASS" : "FAIL") << ","
             << entry.scan_result.scan_time_ms << ","
             << "\"" << entry.scan_result.error_message << "\"\n";
    }
    
    file.close();
    return true;
}

bool ReportGenerator::exportToXML(const std::string& filename) {
    std::ofstream file(config_.output_path + filename);
    
    if (!file.is_open()) {
        return false;
    }
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<report>\n";
    file << "  <report_info>\n";
    file << "    <generated_at>" << generateTimestamp() << "</generated_at>\n";
    file << "    <total_entries>" << entries_.size() << "</total_entries>\n";
    file << "  </report_info>\n";
    file << "  <entries>\n";
    
    for (const auto& entry : entries_) {
        file << "    <entry>\n";
        file << "      <timestamp>" << entry.timestamp << "</timestamp>\n";
        file << "      <data>" << entry.scan_result.data << "</data>\n";
        file << "      <quality_score>" << entry.scan_result.quality.overall_score << "</quality_score>\n";
        file << "      <passed>" << (entry.scan_result.success ? "true" : "false") << "</passed>\n";
        file << "      <scan_time_ms>" << entry.scan_result.scan_time_ms << "</scan_time_ms>\n";
        file << "    </entry>\n";
    }
    
    file << "  </entries>\n";
    file << "</report>\n";
    
    file.close();
    return true;
}

bool ReportGenerator::exportToHTML(const std::string& filename) {
    std::ofstream file(config_.output_path + filename);
    
    if (!file.is_open()) {
        return false;
    }
    
    ReportStatistics stats = getStatistics();
    
    file << "<!DOCTYPE html>\n";
    file << "<html>\n<head>\n";
    file << "  <title>DataMatrix Quality Report</title>\n";
    file << "  <style>\n";
    file << "    body { font-family: Arial, sans-serif; margin: 20px; }\n";
    file << "    table { border-collapse: collapse; width: 100%; }\n";
    file << "    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    file << "    th { background-color: #4CAF50; color: white; }\n";
    file << "    .pass { background-color: #d4edda; }\n";
    file << "    .fail { background-color: #f8d7da; }\n";
    file << "    .summary { margin-bottom: 20px; padding: 15px; background-color: #f5f5f5; }\n";
    file << "  </style>\n";
    file << "</head>\n<body>\n";
    
    file << "<h1>DataMatrix Quality Report</h1>\n";
    
    file << "<div class=\"summary\">\n";
    file << "  <h2>Summary Statistics</h2>\n";
    file << "  <p>Generated: " << generateTimestamp() << "</p>\n";
    file << "  <p>Total Entries: " << stats.total_entries << "</p>\n";
    file << "  <p>Passed: " << stats.passed_count << "</p>\n";
    file << "  <p>Failed: " << stats.failed_count << "</p>\n";
    file << "  <p>Average Quality: " << std::fixed << std::setprecision(2) 
         << stats.average_quality << "%</p>\n";
    file << "</div>\n";
    
    file << "<h2>Detailed Results</h2>\n";
    file << "<table>\n";
    file << "  <tr>\n";
    file << "    <th>Timestamp</th>\n";
    file << "    <th>Data</th>\n";
    file << "    <th>Quality Score</th>\n";
    file << "    <th>Status</th>\n";
    file << "    <th>Scan Time (ms)</th>\n";
    file << "  </tr>\n";
    
    for (const auto& entry : entries_) {
        file << "  <tr class=\"" 
             << (entry.scan_result.success ? "pass" : "fail") << "\">\n";
        file << "    <td>" << entry.timestamp << "</td>\n";
        file << "    <td>" << entry.scan_result.data << "</td>\n";
        file << "    <td>" << std::fixed << std::setprecision(2) 
             << entry.scan_result.quality.overall_score << "</td>\n";
        file << "    <td>" << (entry.scan_result.success ? "PASS" : "FAIL") << "</td>\n";
        file << "    <td>" << entry.scan_result.scan_time_ms << "</td>\n";
        file << "  </tr>\n";
    }
    
    file << "</table>\n";
    file << "</body>\n</html>\n";
    
    file.close();
    return true;
}

ReportGenerator::ReportStatistics ReportGenerator::getStatistics() const {
    ReportStatistics stats;
    
    if (entries_.empty()) {
        return stats;
    }
    
    stats.total_entries = entries_.size();
    stats.first_timestamp = entries_.front().timestamp;
    stats.last_timestamp = entries_.back().timestamp;
    
    double total_quality = 0.0;
    
    for (const auto& entry : entries_) {
        if (entry.scan_result.success) {
            stats.passed_count++;
        } else {
            stats.failed_count++;
        }
        
        total_quality += entry.scan_result.quality.overall_score;
        
        if (entry.scan_result.quality.overall_score < stats.min_quality) {
            stats.min_quality = entry.scan_result.quality.overall_score;
        }
        
        if (entry.scan_result.quality.overall_score > stats.max_quality) {
            stats.max_quality = entry.scan_result.quality.overall_score;
        }
    }
    
    stats.average_quality = total_quality / stats.total_entries;
    
    return stats;
}

void ReportGenerator::clear() {
    entries_.clear();
}

bool ReportGenerator::generateSummaryReport(const std::string& start_date,
                                            const std::string& end_date,
                                            const std::string& output_file) {
    // Фильтрация записей по дате
    std::vector<ReportEntry> filtered_entries;
    
    for (const auto& entry : entries_) {
        if (entry.timestamp >= start_date && entry.timestamp <= end_date) {
            filtered_entries.push_back(entry);
        }
    }
    
    if (filtered_entries.empty()) {
        return false;
    }
    
    // Временная замена entries_
    std::vector<ReportEntry> original_entries = entries_;
    entries_ = filtered_entries;
    
    bool result = exportToHTML(output_file);
    
    // Восстановление original_entries
    entries_ = original_entries;
    
    return result;
}

std::string ReportGenerator::generateTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

std::string ReportGenerator::generateFilename() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, config_.filename_pattern.c_str());
    
    return oss.str();
}

bool ReportGenerator::ensureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}

bool ReportGenerator::saveImage(const cv::Mat& image, const std::string& filename) {
    if (image.empty()) {
        return false;
    }
    
    return cv::imwrite(config_.output_path + filename, image);
}

std::string ReportGenerator::formatJSON(const ReportEntry& entry) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"timestamp\":\"" << entry.timestamp << "\",";
    oss << "\"data\":\"" << entry.scan_result.data << "\",";
    oss << "\"quality\":" << entry.scan_result.quality.overall_score;
    oss << "}";
    return oss.str();
}

std::string ReportGenerator::formatCSV(const ReportEntry& entry) {
    std::ostringstream oss;
    oss << entry.timestamp << ",";
    oss << "\"" << entry.scan_result.data << "\",";
    oss << entry.scan_result.quality.overall_score << ",";
    oss << (entry.scan_result.success ? "PASS" : "FAIL");
    return oss.str();
}

std::string ReportGenerator::formatXML(const ReportEntry& entry) {
    std::ostringstream oss;
    oss << "<entry>";
    oss << "<timestamp>" << entry.timestamp << "</timestamp>";
    oss << "<data>" << entry.scan_result.data << "</data>";
    oss << "<quality>" << entry.scan_result.quality.overall_score << "</quality>";
    oss << "</entry>";
    return oss.str();
}

std::string ReportGenerator::formatHTML(const std::vector<ReportEntry>& entries) {
    std::ostringstream oss;
    oss << "<table>";
    oss << "<tr><th>Timestamp</th><th>Data</th><th>Quality</th><th>Status</th></tr>";
    
    for (const auto& entry : entries) {
        oss << "<tr>";
        oss << "<td>" << entry.timestamp << "</td>";
        oss << "<td>" << entry.scan_result.data << "</td>";
        oss << "<td>" << entry.scan_result.quality.overall_score << "</td>";
        oss << "<td>" << (entry.scan_result.success ? "PASS" : "FAIL") << "</td>";
        oss << "</tr>";
    }
    
    oss << "</table>";
    return oss.str();
}

} // namespace report
