#define NO_MAIN
#include "../../../tools/nfs-trace-server.cpp"

#include "boost-test.hpp"
#include "../face-pair-fixture.hpp"

namespace ndn {
namespace tests {

using namespace ndn::nfs_trace;

BOOST_FIXTURE_TEST_SUITE(TestNfsTraceServer, FacePairFixture)

BOOST_AUTO_TEST_CASE(PrefixFilter)
{
  Server server(face2, "ndn:/NFS", {"ndn:/NFS/P"});

  Interest interest("ndn:/NFS/Q/file1/..../attr");
  interest.setExclude(ServerAction{SA_ATTR, 1417628973, 0});

  bool hasNack = false;
  face1.request(interest,
      bind([] { BOOST_ERROR("DATA"); }),
      bind([&hasNack] { hasNack = true; }),
      bind([] { BOOST_ERROR("TIMEOUT"); }));
  io.poll();
  BOOST_CHECK(hasNack);
}

BOOST_AUTO_TEST_CASE(OpAttr)
{
  Server server(face2, "ndn:/NFS", {"ndn:/NFS/P"});

  Interest interest("ndn:/NFS/P/file1/..../attr");
  interest.setExclude(ServerAction{SA_ATTR, 1417628473, 0});

  bool hasData = false;
  face1.request(interest,
      [&hasData] (const Interest& interest, const Data& data) {
        hasData = true;
        BOOST_CHECK_EQUAL(data.getName().at(-1).toVersion(), 1417628473);
        BOOST_CHECK_EQUAL(data.getContent().value_size(), 84);
      },
      bind([] { BOOST_ERROR("NACK"); }),
      bind([] { BOOST_ERROR("TIMEOUT"); }));
  io.poll();
  BOOST_CHECK(hasData);
}

BOOST_AUTO_TEST_CASE(OpRead)
{
  Server server(face2, "ndn:/NFS", {"ndn:/NFS/P"});

  Name name("ndn:/NFS/P/file1");
  name.appendVersion(1417629185);
  name.appendSegment(2);
  Interest interest(name);
  interest.setExclude(ServerAction{SA_READ, 0, 3969});

  bool hasData = false;
  face1.request(interest,
      [&hasData] (const Interest& interest, const Data& data) {
        hasData = true;
        BOOST_CHECK_EQUAL(data.getName().at(-2).toVersion(), 1417629185);
        BOOST_CHECK_EQUAL(data.getName().at(-1).toSegment(), 2);
        BOOST_CHECK_EQUAL(data.getContent().value_size(), 3969);
      },
      bind([] { BOOST_ERROR("NACK"); }),
      bind([] { BOOST_ERROR("TIMEOUT"); }));
  io.poll();
  BOOST_CHECK(hasData);
}

/*
BOOST_AUTO_TEST_CASE(OpWrite)
{
  // TODO
}
*/

BOOST_AUTO_TEST_CASE(OpReadDir1)
{
  Server server(face2, "ndn:/NFS", {"ndn:/NFS/P"});

  Interest interest("ndn:/NFS/P/dir1/..../dir");
  interest.setExclude(ServerAction{SA_READDIR1, 1417629384, 40});

  bool hasData = false;
  face1.request(interest,
      [&hasData] (const Interest& interest, const Data& data) {
        hasData = true;
        BOOST_CHECK_EQUAL(data.getName().at(-2).toVersion(), 1417629384);
        BOOST_CHECK_EQUAL(data.getName().at(-1).toSegment(), 0);
        BOOST_CHECK_EQUAL(data.getContent().value_size(), 40 * 174);
      },
      bind([] { BOOST_ERROR("NACK"); }),
      bind([] { BOOST_ERROR("TIMEOUT"); }));
  io.poll();
  BOOST_CHECK(hasData);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndn
