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

#ifdef NDNCXXEXT_HAVE_CXX_OVERRIDE_FINAL
#define NDNCXXEXT_DECL_OVERRIDE override
#define NDNCXXEXT_DECL_FINAL override
#else
#define NDNCXXEXT_DECL_OVERRIDE
#define NDNCXXEXT_DECL_FINAL
#endif

#include <ndn-cxx/common.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <boost/asio.hpp>

#endif // NDNCXXEXT_COMMON_HPP
