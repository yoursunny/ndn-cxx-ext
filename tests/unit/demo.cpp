#include "demo.hpp"

#include "boost-test.hpp"

namespace ndn {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestDemo)

BOOST_AUTO_TEST_CASE(Basic)
{
  Demo n = getDemo();
  BOOST_CHECK_EQUAL(n, 50);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
