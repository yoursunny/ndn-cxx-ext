#include "standalone-client-face.hpp"
#include "util/face-trace-writer.hpp"
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace nack_example_producer {

static inline void
onInterest(ClientFace& face, int nackCode, const Name& prefix, const Interest& interest)
{
  std::cout << "INTEREST " << interest.getName() << std::endl;

  if (nackCode == -1) {
    Data data(interest.getName());
    face.reply(interest, data);
  }
  else {
    Nack nack(static_cast<NackCode>(nackCode), interest);
    face.reply(interest, nack);
  }
}

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "USAGE: ./nack-example-producer ndn:/name nackCode" << std::endl;
    return 1;
  }

  boost::asio::io_service io;
  StandaloneClientFace face(io);
  util::FaceTraceWriter::connect(face);

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
