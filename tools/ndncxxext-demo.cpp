#include "demo.hpp"

namespace ndn {
namespace demo {

int
main(int argc, char* argv[])
{
  std::cout << getDemo() << std::endl;
  return 0;
}

} // namespace demo
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::demo::main(argc, argv);
}
