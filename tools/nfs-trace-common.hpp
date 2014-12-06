#ifndef NDNCXXEXT_TOOLS_NFS_TRACE_COMMON_HPP
#define NDNCXXEXT_TOOLS_NFS_TRACE_COMMON_HPP

#include "common.hpp"
#include <sstream>
#include <boost/lexical_cast.hpp>

namespace ndn {
namespace nfs_trace {

enum ServerActionVerb {
  SA_NONE      = 0,
  SA_ATTR      = 1,
  SA_READLINK  = 2,
  SA_READ      = 3,
  SA_WRITE     = 4,
  SA_FETCH     = 5,
  SA_COMMIT    = 6,
  SA_READDIR1  = 7,
  SA_READDIR2  = 8,
  SA_SIMPLECMD = 9
};

static std::vector<std::string> ServerActionVerbStrings = {
  "NONE",
  "ATTR",
  "READLINK",
  "READ",
  "WRITE",
  "FETCH",
  "COMMIT",
  "READDIR1",
  "READDIR2",
  "SIMPLECMD"
};

inline ServerActionVerb
parseServerActionVerb(const std::string& s)
{
  auto it = std::find(ServerActionVerbStrings.begin(), ServerActionVerbStrings.end(), s);
  if (it != ServerActionVerbStrings.end()) {
    return static_cast<ServerActionVerb>(it - ServerActionVerbStrings.begin());
  }
  return SA_NONE;
}

class ServerAction
{
public:
  static ServerAction
  fromExclude(const Exclude& exclude)
  {
    ServerAction sa = { SA_NONE, 0, 0 };
    if (exclude.size() != 1) {
      return sa;
    }

    const name::Component& comp = exclude.begin()->first;
    std::string s(reinterpret_cast<const char*>(comp.value()), comp.value_size());
    size_t pos = s.find(':'), pos2 = std::string::npos;
    std::string verb = (pos == std::string::npos) ? s : s.substr(0, pos);
    sa.verb = parseServerActionVerb(verb);

    switch (sa.verb) {
    case SA_WRITE:
    case SA_FETCH:
    case SA_COMMIT:
    case SA_SIMPLECMD:
      // no arg
      break;
    case SA_ATTR:
    case SA_READLINK:
      // arg1
      sa.arg1 = boost::lexical_cast<uint64_t>(s.substr(pos + 1));
      break;
    case SA_READ:
    case SA_READDIR2:
      // arg2
      sa.arg2 = boost::lexical_cast<size_t>(s.substr(pos + 1));
      break;
    case SA_READDIR1:
      // arg1:arg2
      pos2 = s.rfind(':');
      sa.arg1 = boost::lexical_cast<uint64_t>(s.substr(pos + 1, pos2 - pos - 1));
      sa.arg2 = boost::lexical_cast<size_t>(s.substr(pos2 + 1));
      break;
    default:
      break;
    }
    return sa;
  }

  Exclude
  toExclude() const
  {
    std::stringstream ss;
    ss << ServerActionVerbStrings[verb];
    switch (verb) {
    case SA_WRITE:
    case SA_FETCH:
    case SA_COMMIT:
    case SA_SIMPLECMD:
      // no arg
      break;
    case SA_ATTR:
    case SA_READLINK:
      // arg1
      ss << ':' << arg1;
      break;
    case SA_READ:
    case SA_READDIR2:
      // arg2
      ss << ':' << arg2;
      break;
    case SA_READDIR1:
      // arg1:arg2
      ss << ':' << arg1;
      ss << ':' << arg2;
      break;
    default:
      break;
    }
    std::string s = ss.str();
    Exclude exclude;
    exclude.excludeOne(name::Component(reinterpret_cast<const uint8_t*>(s.data()), s.size()));
    return exclude;
  }

  /** \brief implicitly convertible to Exclude,
   *         so that one can write interest.setExclude(ServerAction{...})
   */
  operator Exclude() const
  {
    return this->toExclude();
  }

public:
  ServerActionVerb verb;
  uint64_t arg1;
  size_t arg2;
};

inline Name&
appendSignature(Name& name)
{
  static const name::Component SIG1("TIME");
  static const name::Component SIG2("NONC");
  static const name::Component SIG3("SIGNATURE-INFO-SIGNATURE-INFO-SIGNATURE-INFO");
  static const name::Component SIG4("SIGNATURE-VALUE-SIGNATURE-VALUE-"
                                    "SIGNATURE-VALUE-SIGNATURE-VALUE");
  name.append(SIG1).append(SIG2).append(SIG3).append(SIG4);
  return name;
}

inline Name
stripSignature(const Name& name)
{
  return name.getPrefix(-ndn::signed_interest::MIN_LENGTH);
}

static const int AUTO_RETRY_LIMIT = 3;

} // namespace nfs_trace
} // namespace ndn

#endif // NDNCXXEXT_TOOLS_NFS_TRACE_COMMON_HPP
