#include "../../../tools/nfs-trace-common.hpp"

#include "boost-test.hpp"

namespace ndn {
namespace tests {

using namespace ndn::nfs_trace;

BOOST_AUTO_TEST_SUITE(TestNfsTraceCommon)

BOOST_AUTO_TEST_CASE(ServerActionNoArg)
{
  ServerAction sa1{SA_WRITE, 0, 0};
  Exclude ex1 = sa1.toExclude();
  ServerAction sa2 = ServerAction::fromExclude(ex1);

  BOOST_CHECK_EQUAL(sa2.verb, sa1.verb);
}

BOOST_AUTO_TEST_CASE(ServerActionArg1)
{
  ServerAction sa1{SA_ATTR, 1417579899, 0};
  Exclude ex1 = sa1.toExclude();
  ServerAction sa2 = ServerAction::fromExclude(ex1);

  BOOST_CHECK_EQUAL(sa2.verb, sa1.verb);
  BOOST_CHECK_EQUAL(sa2.arg1, sa1.arg1);
}

BOOST_AUTO_TEST_CASE(ServerActionArg2)
{
  ServerAction sa1{SA_READ, 0, 4096};
  Exclude ex1 = sa1.toExclude();
  ServerAction sa2 = ServerAction::fromExclude(ex1);

  BOOST_CHECK_EQUAL(sa2.verb, sa1.verb);
  BOOST_CHECK_EQUAL(sa2.arg2, sa1.arg2);
}

BOOST_AUTO_TEST_CASE(ServerActionTwoArgs)
{
  ServerAction sa1{SA_READDIR1, 1417580479, 30};
  Exclude ex1 = sa1.toExclude();
  ServerAction sa2 = ServerAction::fromExclude(ex1);

  BOOST_CHECK_EQUAL(sa2.verb, sa1.verb);
  BOOST_CHECK_EQUAL(sa2.arg1, sa1.arg1);
  BOOST_CHECK_EQUAL(sa2.arg2, sa1.arg2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
