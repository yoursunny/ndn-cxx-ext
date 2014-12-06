#include "nack-enabled-face.hpp"
#include "nfs-trace-common.hpp"
#include "util/request-segments.hpp"

namespace ndn {
namespace nfs_trace {

using ndn::util::requestSegments;
using ndn::util::AutoRetryLimited;

enum NfsProc {
  NFS_NONE,
  NFS_GETATTR,
  NFS_LOOKUP,
  NFS_ACCESS,
  NFS_READLINK,
  NFS_READ,
  NFS_WRITE,
  NFS_READDIRP,
  NFS_SETATTR,
  NFS_CREATE,
  NFS_MKDIR,
  NFS_SYMLINK,
  NFS_REMOVE,
  NFS_RMDIR,
  NFS_RENAME
};

static std::vector<std::string> NfsProcStrings = {
  "none",
  "getattr",
  "lookup",
  "access",
  "readlink",
  "read",
  "write",
  "readdirp",
  "setattr",
  "create",
  "mkdir",
  "symlink",
  "remove",
  "rmdir",
  "rename"
};

NfsProc
parseNfsProc(const std::string& s)
{
  auto it = std::find(NfsProcStrings.begin(), NfsProcStrings.end(), s);
  if (it != NfsProcStrings.end()) {
    return static_cast<NfsProc>(it - NfsProcStrings.begin());
  }
  return NFS_NONE;
}

typedef boost::chrono::time_point<time::system_clock, time::milliseconds> NfsTimestamp;

NfsTimestamp
parseNfsTimestamp(const std::string& s)
{
  double d = boost::lexical_cast<double>(s);
  return NfsTimestamp(time::milliseconds(static_cast<time::milliseconds::rep>(d * 1000000)));
}

struct NfsOp
{
  NfsTimestamp timestamp;
  NfsProc proc;
  std::string path;
  uint64_t version;
  uint64_t segStart;
  uint64_t nSegments;
};

/** \brief parses .ops trace file
 */
class OpsParser : noncopyable
{
public:
  explicit
  OpsParser(std::istream& is);

  NfsOp
  read();

public:
  std::function<bool(const NfsTimestamp&)> acceptTimestamp;

private:
  std::istream& m_is;
};

OpsParser::OpsParser(std::istream& is)
  : acceptTimestamp(bind([] { return true; }))
  , m_is(is)
{
}

NfsOp
OpsParser::read()
{
  while (true) {
    NfsOp op = {NfsTimestamp(), NFS_NONE, "", 0, 0, 0};
    if (!m_is) {
      return op;
    }

    static std::string line;
    std::getline(m_is, line);

    size_t pos1 = line.find(','),
           pos2 = line.find(',', pos1 + 1),
           pos3 = line.find(',', pos2 + 1),
           pos4 = line.find(',', pos3 + 1),
           pos5 = line.find(',', pos4 + 1);
    if (pos1 == std::string::npos ||
        pos2 == std::string::npos ||
        pos3 == std::string::npos ||
        pos4 == std::string::npos ||
        pos5 == std::string::npos) {
      continue; // bad input line
    }

    op.timestamp = parseNfsTimestamp(line.substr(0, pos1));
    if (!acceptTimestamp(op.timestamp)) {
      continue;
    }

    op.proc = parseNfsProc(line.substr(pos1 + 1, pos2 - pos1 - 1));
    if (op.proc == NFS_NONE) {
      continue; // bad input line
    }

    op.path = line.substr(pos2 + 1, pos3 - pos2 - 1);

    if (op.proc == NFS_READ || op.proc == NFS_WRITE || op.proc == NFS_READDIRP) {
      op.version = parseNfsTimestamp(line.substr(pos3 + 1, pos4 - pos3 - 1))
                   .time_since_epoch().count();
      op.segStart = boost::lexical_cast<uint64_t>(line.substr(pos4 + 1, pos5 - pos4 - 1));
      op.nSegments = boost::lexical_cast<uint64_t>(line.substr(pos5 + 1));
    }
    return op;
  }
  BOOST_ASSERT(false);
  return {NfsTimestamp(), NFS_NONE, "", 0, 0, 0};
}

int
client_main(int argc, char* argv[])
{
  return 0;
}

} // namespace nfs_trace
} // namespace ndn

#ifndef NO_MAIN

int
main(int argc, char* argv[])
{
  return ndn::nfs_trace::client_main(argc, argv);
}

#endif // NO_MAIN
