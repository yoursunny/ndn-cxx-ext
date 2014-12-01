#include "nack-enabled-face.hpp"
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace nack_example_producer {

static inline void
onInterest(NackEnabledFace& face, int nackCode, const Name& prefix, const Interest& interest)
{
  std::cout << "INTEREST " << interest.getName() << std::endl;

  Nack nack(static_cast<NackCode>(nackCode), interest);
  face.reply(nack);
}

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "USAGE: ./nack-example-producer ndn:/name nackCode" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  NackEnabledFace face(io);

  face.listen(Name(argv[1]), bind(&onInterest, ref(face), boost::lexical_cast<int>(argv[2]), _1, _2));
  io.run();

  return 0;
}

} // namespace nack_example_producer
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::nack_example_producer::main(argc, argv);
}
