#ifndef NDNCXXEXT_COMMON_HPP
#define NDNCXXEXT_COMMON_HPP

#include "ndn-cxx-ext-config.hpp"

#ifdef NDNCXXEXT_WITH_TESTS
#define NDNCXXEXT_VIRTUAL_WITH_TESTS virtual
#define NDNCXXEXT_PUBLIC_WITH_TESTS_ELSE_PROTECTED public
#define NDNCXXEXT_PUBLIC_WITH_TESTS_ELSE_PRIVATE public
#define NDNCXXEXT_PROTECTED_WITH_TESTS_ELSE_PRIVATE protected
#else
#define NDNCXXEXT_VIRTUAL_WITH_TESTS
#define NDNCXXEXT_PUBLIC_WITH_TESTS_ELSE_PROTECTED protected
#define NDNCXXEXT_PUBLIC_WITH_TESTS_ELSE_PRIVATE private
#define NDNCXXEXT_PROTECTED_WITH_TESTS_ELSE_PRIVATE private
#endif

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>

#endif // NDNCXXEXT_COMMON_HPP
