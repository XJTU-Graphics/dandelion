#ifndef DANDELION_UTILS_LOGGER_H
#define DANDELION_UTILS_LOGGER_H

#include <memory>
#include <string>

#include "formatter.hpp"
#include <spdlog/spdlog.h>

/*!
 * \file utils/logger.h
 * \ingroup utils
 * \brief 声明了创建 / 获取 logger 用的工具函数。
 */

/*!
 * \~chinese
 * \brief 获取指定名称的 logger 。
 *
 * 如果指定名称的 logger 尚不存在，这个函数会创建它；反之则返回这个 logger
 * 的指针。每个 logger 都有两个 sink ，分别输出到 stdout 和 *dandelion.log*
 * 中。由于目前尚不需要多线程共享 logger ，所有的 sink 都是单线程版本。
 */
std::shared_ptr<spdlog::logger> get_logger(const std::string& name);

#endif // DANDELION_UTILS_LOGGER_H
