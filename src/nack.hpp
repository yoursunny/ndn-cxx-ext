#ifndef NDNCXXEXT_NACK_HPP
#define NDNCXXEXT_NACK_HPP

#include "common.hpp"
#include <ndn-cxx/interest.hpp>

namespace ndn {

/** \brief represents a NACK packet
 */
class Nack : public TagHost
{
public:
  enum NackCode {
    NONE      = 0,
    DUPLICATE = 160,
    GIVEUP    = 161,
    NODATA    = 162,
    BUSY      = 163
  };

  Nack()
    : m_code(NONE)
  {
  }

  Nack(NackCode code, const Interest& interest)
    : m_code(code)
    , m_interest(interest)
  {
  }

  NackCode
  getCode() const
  {
    return m_code;
  }

  const Interest&
  getInterest() const
  {
    return m_interest;
  }

public:
  /** \brief encode NACK to an Interest (hack)
   *
   *  ndn:/localhop/NACK/<interest/name/components>/<code>/<selectors>/<nonce>
   */
  Interest
  encode() const;

  /** \brief decode NACK from an Interest (hack)
   *
   *  ndn:/localhop/NACK/<interest/name/components>/<code>/<selectors>/<nonce>
   *  \retval true success
   */
  bool
  decode(const Interest& packet);

public:
  /** \brief ndn:/localhop/NACK
   */
  static const Name PREFIX;

private:
  NackCode m_code;
  Interest m_interest;
};

typedef Nack::NackCode NackCode;

std::ostream&
operator<<(std::ostream& os, NackCode code);

} // namespace ndn

#endif // NDNCXXEXT_NACK_HPP
