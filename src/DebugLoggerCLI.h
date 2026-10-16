/**
 * @file DebugLoggerCLI.h
 * @brief CommandInterface integration for DebugLogger runtime management
 * @author Felipe Dadison
 * @version 1.1.0
 * @date 2026-10-16
 *
 * Optional integration: include this header to register logger management commands
 * with CommandInterface. Requires the CommandInterface library (1.1.0 or newer).
 * Projects that don't include this header don't depend on CommandInterface.
 */

#pragma once

#include "CommandInterface.h"
#include "DebugLogger.h"

/**
 * @brief Register DebugLogger management commands with CommandInterface
 *
 * Available commands:
 * - LOGGER_SET_LEVEL\level : Set console log level (0-4)
 * - LOGGER_GET_LEVEL : Get current console log level
 * - LOGGER_SET_FILE_LEVEL\level : Set file log level (0-4)
 * - LOGGER_GET_FILE_LEVEL : Get file log level
 * - LOGGER_CLEAR_FILE : Clear all file logs
 * - LOGGER_GET_FILE : Display all file logs
 * - LOGGER_ENABLE_TAG\tag : Add a tag to the enabled tags (only enabled tags are shown)
 * - LOGGER_DISABLE_TAG\tag : Remove a tag from the enabled tags
 * - LOGGER_CLEAR_TAG : Clear the tag filter (all tags shown)
 *
 * @param logger Reference to DebugLogger instance to manage
 */
inline void registerLoggerCLI(DebugLogger& logger) {
    DebugLogger* log = &logger;

    Cli.registerTypedCommand<int>("LOGGER_SET_LEVEL", "Sets the logger level (0 None, 1 Error, 2 Warn, 3 Info, 4 Debug).", {{"level", "int"}}, [log](int level) {
        if (level < 0 || level > 4) Cli.println("Invalid input range (0 to 4).");
        else {
            log->setLevel((LogLevel)level);
            Cli.println("Logger level: " + String(level));
        }
    });

    Cli.registerTypedCommand<>("LOGGER_GET_LEVEL", "Gets the logger level (0 to 4).", {}, [log]() {
        Cli.println("Logger level: " + String((int)log->getLevel()));
    });

    Cli.registerTypedCommand<int>("LOGGER_SET_FILE_LEVEL", "Sets the logger file level (0 None, 1 Error, 2 Warn, 3 Info, 4 Debug).", {{"level", "int"}}, [log](int level) {
        if (level < 0 || level > 4) Cli.println("Invalid input range (0 to 4).");
        else {
            log->setFileLevel((LogLevel)level);
            Cli.println("File logger level: " + String(level));
        }
    });

    Cli.registerTypedCommand<>("LOGGER_GET_FILE_LEVEL", "Gets the logger file level (0 to 4).", {}, [log]() {
        Cli.println("File logger level: " + String((int)log->getFileLevel()));
    });

    Cli.registerTypedCommand<>("LOGGER_CLEAR_FILE", "Clears the logger file.", {}, [log]() {
        if (log->clearFileLogs()) {
            Cli.println("Done.");
        } else {
            Cli.println("Fail to clear file.");
        }
    });

    Cli.registerTypedCommand<>("LOGGER_GET_FILE", "Gets the logger file.", {}, [log]() {
        auto logs = log->readAllFileLogs();
        for (auto &line : logs) {
            Cli.println(line);
        }
        Cli.println("");
    });

    Cli.registerTypedCommand<String>("LOGGER_ENABLE_TAG", "Shows only enabled tags.", {{"tag", "String"}}, [log](String tag) {
        log->enableTag(tag);
        Cli.println(tag + " enabled.");
    });

    Cli.registerTypedCommand<String>("LOGGER_DISABLE_TAG", "Removes a tag from the enabled tags.", {{"tag", "String"}}, [log](String tag) {
        log->disableTag(tag);
        Cli.println(tag + " disabled.");
    });

    Cli.registerTypedCommand<>("LOGGER_CLEAR_TAG", "Clears the tag filter (all tags shown).", {}, [log]() {
        log->clearTags();
        Cli.println("Tag filter cleared, all tags shown.");
    });
}