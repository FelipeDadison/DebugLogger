/**
 * @file DebugLogger.cpp
 * @brief Implementation of advanced logging system for ESP32
 * @author Felipe Dadison
 * @version 1.0.0
 * @date 2026-03-11
 * 
 * Implements multi-level console and file logging with ANSI colors,
 * tag filtering, and optional timestamp support.
 */

#include "DebugLogger.h"
#include <stdarg.h>

DebugLogger::DebugLogger() : _currentLevel(LogLevel::INFO) {}

void DebugLogger::addOutput(Print* output) {
    _outputs.push_back(output);
}

void DebugLogger::clearOutputs() {
    _outputs.clear();
}

void DebugLogger::setLevel(LogLevel level) {
    _currentLevel = level;
}

LogLevel DebugLogger::getLevel() const {
    return _currentLevel;
}

void DebugLogger::enableTag(const String& tag) {
    _enabledTags.insert(tag);
}

void DebugLogger::disableTag(const String& tag) {
    _enabledTags.erase(tag);
}

void DebugLogger::clearTags() {
    _enabledTags.clear();
}

bool DebugLogger::beginFileLogging(FS &fs, LogLevel level, const char* filePath, size_t maxLines) {
    _fs = &fs;
    _logFilePath = filePath;
    _fileLogLevel = level;
    _maxFileLines = maxLines;
    _fileLoggingEnabled = true;

    if (!_fs->exists(_logFilePath)) {
        File file = _fs->open(_logFilePath, FILE_WRITE);
        if (!file) return false;
        file.close();
    }
    return true;
}

void DebugLogger::stopFileLogging() {
    _fileLoggingEnabled = false;
    _pendingFileLogs.clear();
}

void DebugLogger::enableFileTag(const String& tag) { _fileTags.insert(tag); }
void DebugLogger::disableFileTag(const String& tag) { _fileTags.erase(tag); }
void DebugLogger::clearFileTags() { _fileTags.clear(); }
LogLevel DebugLogger::getFileLevel() const { return _fileLogLevel; }

std::vector<String> DebugLogger::readAllFileLogs() const {
    std::vector<String> lines;
    if (!_fileLoggingEnabled || !_fs) return lines;
    if (!_fs->exists(_logFilePath)) return lines;

    File file = _fs->open(_logFilePath, FILE_READ);
    if (!file) return lines;

    while (file.available()) {
        lines.push_back(file.readStringUntil('\n'));
    }
    file.close();
    return lines;
}

bool DebugLogger::clearFileLogs() {
    if (!_fileLoggingEnabled || !_fs) return false;
    File file = _fs->open(_logFilePath, FILE_WRITE);
    if (!file) return false;
    file.close();
    return true;
}

bool DebugLogger::update() {
    if (!_fileLoggingEnabled || !_fs) return false;
    if (_pendingFileLogs.empty()) return false;

    File file = _fs->open(_logFilePath, FILE_READ);
    if (!file) return false;

    std::vector<String> lines;
    while (file.available()) {
        lines.push_back(file.readStringUntil('\n'));
    }
    file.close();

    lines.insert(lines.end(), _pendingFileLogs.begin(), _pendingFileLogs.end());
    _pendingFileLogs.clear();

    if (lines.size() > _maxFileLines) {
        size_t start = lines.size() - _maxFileLines;
        lines.erase(lines.begin(), lines.begin() + start);
    }

    file = _fs->open(_logFilePath, FILE_WRITE);
    if (!file) return false;
    for (auto &line : lines) {
        file.println(line);
    }
    file.close();

    return true;
}

void DebugLogger::error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::ERROR, nullptr, format, args);
    va_end(args);
}

void DebugLogger::warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::WARN, nullptr, format, args);
    va_end(args);
}

void DebugLogger::info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::INFO, nullptr, format, args);
    va_end(args);
}

void DebugLogger::debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::DEBUG, nullptr, format, args);
    va_end(args);
}

void DebugLogger::errorTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::ERROR, tag, format, args);
    va_end(args);
}

void DebugLogger::warnTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::WARN, tag, format, args);
    va_end(args);
}

void DebugLogger::infoTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::INFO, tag, format, args);
    va_end(args);
}

void DebugLogger::debugTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::DEBUG, tag, format, args);
    va_end(args);
}

void DebugLogger::infoEveryN(const char* tag, unsigned long interval_ms, const char* format, ...) {
    va_list args;
    va_start(args, format);

    unsigned long now = millis();
    String tagStr = tag ? tag : "GLOBAL";
    if (now - _lastLogTimes_ms[tagStr] >= interval_ms) {
        _lastLogTimes_ms[tagStr] = now;
        vlog(LogLevel::INFO, tag, format, args);
    }

    va_end(args);
}

const char* DebugLogger::getLevelLabel(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "\033[1;31m[ERROR]\033[0m"; // Red
        case LogLevel::WARN:  return "\033[1;33m[WARN]\033[0m";  // Yellow
        case LogLevel::INFO:  return "\033[1;32m[INFO]\033[0m";  // Green
        case LogLevel::DEBUG: return "\033[1;34m[DEBUG]\033[0m"; // Blue
        default: return "[UNKNOWN]";
    }
}

const char* DebugLogger::getLevelLabelPlain(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "[ERROR]"; 
        case LogLevel::WARN:  return "[WARN]";  
        case LogLevel::INFO:  return "[INFO]";  
        case LogLevel::DEBUG: return "[DEBUG]"; 
        default: return "[UNKNOWN]";
    }
}

void DebugLogger::vlog(LogLevel level, const char* tag, const char* format, va_list args) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);

    if (level <= _currentLevel && !_outputs.empty()) {
        String tagStr = tag ? String(tag) : String();
        if (tagStr.isEmpty() || (!_enabledTags.empty() && _enabledTags.find(tagStr) != _enabledTags.end())) {

            const char* levelLabel = getLevelLabel(level);

            for (auto* output : _outputs) {
                if (tag) output->printf("%s \033[1;36m[%s]\033[0m %s\n", levelLabel, tag, buf);
                else output->printf("%s %s\n", levelLabel, buf);
            }
        }
    }

    if (_fileLoggingEnabled && _fs && level <= _fileLogLevel) {
        String tagStr = tag ? String(tag) : String();
        if (tagStr.isEmpty() || (!_fileTags.empty() && _fileTags.find(tagStr) != _fileTags.end())) {
            char timeBuf[20];
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 1000)) {
                strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            } else {
                strcpy(timeBuf, "1970-01-01 00:00:00");
            }

            String logLine;
            if (!tagStr.isEmpty()) {
                logLine = String(timeBuf) + " " + getLevelLabelPlain(level) + " [" + tagStr + "] " + buf;
            } else {
                logLine = String(timeBuf) + " " + getLevelLabelPlain(level) + " " + buf;
            }

            _pendingFileLogs.push_back(logLine);
        }
    }
}

DebugLogger Logger;