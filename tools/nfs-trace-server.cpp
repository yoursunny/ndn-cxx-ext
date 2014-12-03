#include "nack-enabled-face.hpp"
#include "nfs-trace-common.hpp"

namespace ndn {
namespace nfs_trace {

class Server
{
public:
  Server(NackEnabledFace& face, const Name& prefix);

private:
  void
  processInterest(const Interest& interest);
};

Server::Server(NackEnabledFace& face, const Name& prefix)
{
}

int
server_main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "USAGE: ./fake-file-server ndn:/name" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  NackEnabledFace face(io);
  Server server(face, Name(argv[1]));
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
