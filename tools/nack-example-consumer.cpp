#include "nack-enabled-face.hpp"

namespace ndn {
namespace nack_example_consumer {

static inline void
onData(const Interest& interest, const Data& data)
{
  std::cout << "DATA " << data.getName() << std::endl;
  exit(0);
}

static inline void
onNack(const Interest& interest, const Nack& nack)
{
  std::cout << "NACK[" << nack.getCode() << "] " << nack.getInterest().getName() << std::endl;
  exit(0);
}

static inline void
onTimeout(const Interest& interest)
{
  std::cout << "TIMEOUT" << std::endl;
  exit(0);
}

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "USAGE: ./nack-example-consumer ndn:/name" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  NackEnabledFace face(io);
  face.request(Name(argv[1]), &onData, &onNack, &onTimeout);
  io.run();

  return 0;
}

} // namespace nack_example_consumer
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::nack_example_consumer::main(argc, argv);
}
