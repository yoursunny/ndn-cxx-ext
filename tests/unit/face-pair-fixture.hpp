#ifndef NDNCXXEXT_TESTS_FACE_PAIR_FIXTURE_HPP
#define NDNCXXEXT_TESTS_FACE_PAIR_FIXTURE_HPP

#include "transport-pair.hpp"

namespace ndn {
namespace tests {

class FacePairFixture
{
protected:
  FacePairFixture()
    : m_transportPair(makeTransportPair())
    , face1(io, std::move(m_transportPair.first))
    , face2(io, std::move(m_transportPair.second))
  {
  }

protected:
  boost::asio::io_service io;

private:
  std::pair<unique_ptr<Transport>, unique_ptr<Transport>> m_transportPair;

protected:
  NackEnabledFace face1;
  NackEnabledFace face2;
};

} // namespace tests
} // namespace ndn

#endif // NDNCXXEXT_TESTS_FACE_PAIR_FIXTURE_HPP
