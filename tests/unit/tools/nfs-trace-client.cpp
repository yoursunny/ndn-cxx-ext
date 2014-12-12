#define NO_MAIN
#include "../../../tools/nfs-trace-client.cpp"

#include "boost-test.hpp"
#include "../face-pair-fixture.hpp"

namespace ndn {
namespace tests {

using namespace ndn::nfs_trace;

BOOST_FIXTURE_TEST_SUITE(TestNfsTraceClient, FacePairFixture)

BOOST_AUTO_TEST_CASE(ParseOps)
{
  std::stringstream input(
    "BAD\n"
    "1417835239.000000,getattr,/home/u1/f1,,,\n"
    "1417835240.000001,BAD,/home/u2/f2,,,\n"
    "1417835241.000003,read,/home/u3/f3,1417835241.000002,2,3\n"
  );

  OpsParser parser(input);

  auto rec1 = parser.read();
  BOOST_CHECK_EQUAL(rec1.timestamp, 1417835239000000);
  BOOST_CHECK_EQUAL(rec1.proc, NFS_GETATTR);
  BOOST_CHECK_EQUAL(rec1.path, "/home/u1/f1");

  auto rec2 = parser.read();
  BOOST_CHECK_EQUAL(rec2.timestamp, 1417835241000003);
  BOOST_CHECK_EQUAL(rec2.proc, NFS_READ);
  BOOST_CHECK_EQUAL(rec2.path, "/home/u3/f3");
  BOOST_CHECK_EQUAL(rec2.version, 1417835241000002);
  BOOST_CHECK_EQUAL(rec2.segStart, 2);
  BOOST_CHECK_EQUAL(rec2.nSegments, 3);

  BOOST_CHECK_EQUAL(parser.read().proc, NFS_NONE);
  BOOST_CHECK_EQUAL(parser.read().proc, NFS_NONE);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
