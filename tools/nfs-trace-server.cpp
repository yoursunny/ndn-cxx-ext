#include "nack-enabled-face.hpp"
#include "nfs-trace-common.hpp"

namespace ndn {
namespace nfs_trace {

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

private:
  NackEnabledFace& m_face;
  const std::vector<Name> m_prefixes;
  uint8_t m_payloadBuffer[ndn::MAX_NDN_PACKET_SIZE];
};

Server::Server(NackEnabledFace& face, const Name& prefix, const std::vector<Name>& prefixes)
  : m_face(face)
  , m_prefixes(prefixes)
{
  std::fill_n(m_payloadBuffer, sizeof(m_payloadBuffer), 0xBB);
  m_face.listen(prefix, bind(&Server::processInterest, this, _2));
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
    // TODO fetch
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
