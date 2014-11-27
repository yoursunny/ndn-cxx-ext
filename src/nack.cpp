#include "nack.hpp"

namespace ndn {

const Name Nack::PREFIX("ndn:/localhop/NACK");

Interest
Nack::encode() const
{
  Name name(PREFIX);
  name.append(m_interest.getName());
  name.appendNumber(m_code);
  name.append(m_interest.getSelectors().wireEncode());
  name.appendNumber(m_interest.getNonce());

  Interest packet(name);
  packet.setInterestLifetime(time::milliseconds(10));
  return packet;
}

bool
Nack::decode(const Interest& packet)
{
  const Name& name = packet.getName();
  if (!PREFIX.isPrefixOf(name) || name.size() < 5) {
    return false;
  }

  m_code = static_cast<NackCode>(name.at(-3).toNumber());
  m_interest.setName(name.getSubName(2, name.size() - 5));
  m_interest.setSelectors(ndn::Selectors(name.at(-2).blockFromValue()));
  m_interest.setNonce(name.at(-1).toNumber());
  return true;
}

} // namespace ndn
