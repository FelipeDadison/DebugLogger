/**
 * @file LoggerCLI.h
 * @brief CommandInterface integration for DebugLogger runtime management
 * @author Felipe Dadison
 * @version 1.0.0
 * @date 2026-03-11
 * 
 * Provides CLI commands for runtime logger configuration when CommandInterface
 * library is available. Only compiles if CommandInterface is included first.
 */

#pragma once

#if __has_include("CommandInterface.h")

class DebugLogger;

/**
 * @brief Register DebugLogger management commands with CommandInterface
 * 
 * Available commands:
 * - LOGGER_SET_LEVEL\level : Set console log level (0-4)
 * - LOGGER_GET_LEVEL : Get current console log level
 * - LOGGER_GET_FILE_LEVEL : Get file log level
 * - LOGGER_CLEAR_FILE : Clear all file logs
 * - LOGGER_GET_FILE : Display all file logs
 * - LOGGER_ENABLE_TAG\tag : Enable filtering for tag
 * - LOGGER_DISABLE_TAG\tag : Disable filtering for tag
 * - LOGGER_CLEAR_TAG : Clear all tag filters
 * 
 * @param logger Reference to DebugLogger instance to manage
 * @note Only available if CommandInterface library is included before DebugLogger
 */
void registerLoggerCLI(DebugLogger& logger);

#endif