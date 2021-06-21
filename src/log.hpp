#pragma once

/**
 * @file log.hpp
 * @author tonghao.yuan (yuantonghao@gmail.com)
 * @brief   thin warpper of spdlog
 * @version 0.1
 * @date 2021-05-27
 *
 *
 */

#define LOGD SPDLOG_DEBUG
#define LOGI SPDLOG_INFO
#define LOGW SPDLOG_WARN
#define LOGE SPDLOG_ERROR

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <string_view>

#include "dcmtk/oflog/logger.h"
#include "dcmtk/oflog/loglevel.h"
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"

class Log {
 public:
  Log() {
    auto std_cout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("DCMTK tutorial", std_cout_sink));

    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
    spdlog::set_pattern("%L %Y-%m-%d@%T.%e %s:%# => %v");
    LOGI("{} started", FILE_NAME);
    dcmtk::log4cplus::Logger::getRoot().setLogLevel(dcmtk::log4cplus::TRACE_LOG_LEVEL);
  }

  Log(const Log&) = delete;
  Log(Log&&) = delete;
  auto operator=(const Log&) -> Log& = delete;
  auto operator=(Log&&) -> Log& = delete;

  ~Log() { LOGI("{} terminated", FILE_NAME); }

 private:
  static const Log log_;
};

const inline Log Log::log_;