#ifndef NDNCXXEXT_UTIL_LOGGER_HPP
#define NDNCXXEXT_UTIL_LOGGER_HPP

#include "common.hpp"
#include <ndn-cxx/util/time.hpp>

namespace ndn {

const char*
getLoggerTimestamp();

} // namespace ndn

#define LOG(x) \
do { \
  std::cout << ::ndn::getLoggerTimestamp() << " " << x << std::endl; \
} while (0);

#endif // NDNCXXEXT_UTIL_LOGGER_HPP
