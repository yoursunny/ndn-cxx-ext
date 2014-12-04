#include "nack-enabled-face.hpp"
#include "nfs-trace-common.hpp"
#include "util/request-segments.hpp"

namespace ndn {
namespace nfs_trace {

using ndn::util::requestSegments;
using ndn::util::AutoRetryLimited;

class Server
{
public:
  /** \param prefix Name prefix to register with forwarder
   *  \param prefixes Name prefix to serve
   */
  Server(NackEnabledFace& face, const Name& prefix, const std::vector<Name>& prefixes);

private:
  void
  processInterest(const Interest& interest);

  /** \brief answers a request from trace replay
   */
  void
  processRequest(const Interest& interest, const ServerAction& sa);

  void
  answerSimple(const Interest& interest, const std::vector<name::Component>& appendToName,
               size_t payloadSize);

  /** \brief fetches from client after a WRITE request
   */
  void
  writeFetch(const Interest& interest);

private:
  NackEnabledFace& m_face;
  const Name m_prefix;
  const std::vector<Name> m_prefixes;
  uint8_t m_payloadBuffer[ndn::MAX_NDN_PACKET_SIZE];
};

Server::Server(NackEnabledFace& face, const Name& prefix, const std::vector<Name>& prefixes)
  : m_face(face)
  , m_prefix(prefix)
  , m_prefixes(prefixes)
{
  std::fill_n(m_payloadBuffer, sizeof(m_payloadBuffer), 0xBB);
  m_face.listen(m_prefix, bind(&Server::processInterest, this, _2));
}

void
Server::processInterest(const Interest& interest)
{
  if (!std::any_of(m_prefixes.begin(), m_prefixes.end(),
                   bind(&Name::isPrefixOf, _1, interest.getName()))) {
    // not served by this server
    m_face.reply(Nack(Nack::NODATA, interest));
    return;
  }

  ServerAction sa = ServerAction::fromExclude(interest.getExclude());
  if (sa.verb != SA_NONE) {
    this->processRequest(interest, sa);
  }
}

void
Server::processRequest(const Interest& interest, const ServerAction& sa)
{
  switch (sa.verb) {
  case SA_ATTR:
    this->answerSimple(interest, {name::Component::fromVersion(sa.arg1)}, 84);
    break;
  case SA_READLINK:
    this->answerSimple(interest, {name::Component::fromVersion(sa.arg1)}, 160);
    break;
  case SA_READ:
    this->answerSimple(interest, {}, sa.arg2);
    break;
  case SA_WRITE:
    this->answerSimple(interest, {}, 108);
    this->writeFetch(interest);
    break;
  case SA_FETCH: // fetch is from server to client
    break;
  case SA_COMMIT:
    this->answerSimple(interest, {}, 100);
    break;
  case SA_READDIR1:
    this->answerSimple(interest,
        {name::Component::fromVersion(sa.arg1), name::Component::fromSegment(0)},
        174 * sa.arg2);
    break;
  case SA_READDIR2:
    this->answerSimple(interest, {}, 174 * sa.arg2);
    break;
  case SA_SIMPLECMD:
    this->answerSimple(interest, {}, 248);
    break;
  default:
    break;
  }
}

void
Server::answerSimple(const Interest& interest, const std::vector<name::Component>& appendToName,
                     size_t payloadSize)
{
  Name dataName = interest.getName();
  for (const name::Component& comp : appendToName) {
    dataName.append(comp);
  }
  Data data(dataName);
  data.setContent(m_payloadBuffer, std::min(payloadSize, sizeof(m_payloadBuffer)));
  m_face.reply(data);
}

void
Server::writeFetch(const Interest& interest)
{
  const size_t N_COMMAND_COMPONENTS = 3;
  // "./write/{client-host}:{%FD mtime}:{%00 first-seg}:{%00 last-seg}"

  if (interest.getName().size() <=
      m_prefix.size() + ndn::signed_interest::MIN_LENGTH + N_COMMAND_COMPONENTS) {
    return;
  }

  Name command = stripSignature(interest.getName());
  const name::Component& params = command.at(-1);
  std::string s(reinterpret_cast<const char*>(params.value()), params.value_size());
  size_t pos1 = s.find(':'), pos2 = s.find(':', pos1 + 1), pos3 = s.find(':', pos2 + 1);
  if (pos3 == std::string::npos || pos3 != s.rfind(':')) {
    return;
  }

  Name client(s.substr(0, pos1));
  uint64_t mtime = boost::lexical_cast<uint64_t>(s.substr(pos1 + 1, pos2 - pos1 - 1));
  uint64_t first = boost::lexical_cast<uint64_t>(s.substr(pos2 + 1, pos3 - pos2 - 1));
  uint64_t last  = boost::lexical_cast<uint64_t>(s.substr(pos3 + 1));

  Name path = command.getSubName(m_prefix.size(),
                                 command.size() - N_COMMAND_COMPONENTS - m_prefix.size());

  requestSegments(m_face, Name(client).append("NFS").append(path).appendVersion(mtime),
                  {first, last}, nullptr, nullptr, nullptr,
                  AutoRetryLimited(AUTO_RETRY_LIMIT),
                  [] (Interest& interest) { interest.setExclude(ServerAction{SA_FETCH, 0, 0}); });
}

int
server_main(int argc, char* argv[])
{
  boost::asio::io_service io;
  NackEnabledFace face(io);
  Server server(face, "ndn:/NFS", {"ndn:/NFS/home"});
  io.run();

  return 0;
}

} // namespace nfs_trace
} // namespace ndn

#ifndef NO_MAIN

int
main(int argc, char* argv[])
{
  return ndn::nfs_trace::server_main(argc, argv);
}

#endif // NO_MAIN