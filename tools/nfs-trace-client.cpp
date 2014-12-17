#include "nack-enabled-face.hpp"
#include "nfs-trace-common.hpp"
#include <unordered_map>
#include <ndn-cxx/util/event-emitter.hpp>
#include "util/request-segments.hpp"

namespace ndn {
namespace nfs_trace {

using ndn::util::EventEmitter;
using ndn::util::requestSegments;
using ndn::util::AutoRetryLimited;

// use system_clock so that logs can be correlated across machines
typedef time::system_clock EmulationClock;
typedef EmulationClock::TimePoint EmulationTime;

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

typedef uint64_t NfsTimestamp; // microseconds from epoch

NfsTimestamp
parseNfsTimestamp(const std::string& s)
{
  double d = boost::lexical_cast<double>(s);
  return static_cast<NfsTimestamp>(d * 1000000);
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

std::ostream&
operator<<(std::ostream& os, const NfsOp& op)
{
  os << op.timestamp << ','
     << NfsProcStrings[op.proc] << ','
     << op.path << ','
     << op.version << ','
     << op.segStart << ','
     << op.nSegments;
  return os;
}

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
      op.version = parseNfsTimestamp(line.substr(pos3 + 1, pos4 - pos3 - 1));
      op.segStart = boost::lexical_cast<uint64_t>(line.substr(pos4 + 1, pos5 - pos4 - 1));
      op.nSegments = boost::lexical_cast<uint64_t>(line.substr(pos5 + 1));
    }

    if (op.version == 0) {
      // TODO reprocess the trace for accurate ctime or mtime
      op.version = op.timestamp;
    }

    return op;
  }
  BOOST_ASSERT(false);
  return {NfsTimestamp(), NFS_NONE, "", 0, 0, 0};
}

class Client
{
public:
  /** \param serverPrefix Name prefix of NFS servers
   *  \param clientHost prefix of this client host
   */
  Client(NackEnabledFace& face, const Name& serverPrefix, const Name& clientHost);

  void
  startOp(const NfsOp& op);

  /** \brief call this periodically, and sometime after last operation,
   *         to clean up internal states
   */
  void
  periodicalCleanup();

private:
  Interest
  makeCommand(const std::string& path, const std::vector<name::Component>& appendToName,
              const ServerAction& sa, bool needSignature);

  /** \brief send a single-Interest command
   */
  void
  sendCommand(const NfsOp& op, const std::vector<name::Component>& appendToName,
              const ServerAction& sa, bool needSignature);

  void
  processIncomingInterest(const Interest& interest);

  void
  startRead(const NfsOp& op);

  void
  startReadDir(const NfsOp& op);

private: // WRITE
  struct WriteProcess
  {
    EmulationTime start;
    NfsOp op;
    EmulationTime lastFetch;
    std::set<uint64_t> fetchedSegments;
  };

  void
  startWrite(const NfsOp& op);

  void
  processFetchInterest(const Interest& interest, const ServerAction& sa);

  void
  finishWrite(const Name& fetchPrefix);

  /** \brief fail WRITEs where no fetch Interest has been received within FETCH_MAX_GAP
   */
  void
  expireWrites();

public:
  EventEmitter<NfsOp, EmulationTime, EmulationTime> opSuccess;
  EventEmitter<NfsOp, EmulationTime, EmulationTime> opFailure;

private:
  NackEnabledFace& m_face;
  Name m_serverPrefix;
  std::string m_clientHost;
  Name m_clientPrefix;
  uint8_t m_payloadBuffer[ndn::MAX_NDN_PACKET_SIZE];
  std::unordered_map<Name, WriteProcess> m_writes;
  static const int SEGMENT_SIZE = 4096;
  static const int DIR_PER_SEGMENT = 32;
  static const EmulationClock::Duration FETCH_MAX_GAP;
};
const EmulationClock::Duration Client::FETCH_MAX_GAP = time::seconds(4);

Client::Client(NackEnabledFace& face, const Name& serverPrefix, const Name& clientHost)
  : m_face(face)
  , m_serverPrefix(serverPrefix)
  , m_clientHost(clientHost.toUri())
  , m_clientPrefix(Name(clientHost).append("NFS"))
{
  std::fill_n(m_payloadBuffer, sizeof(m_payloadBuffer), 0xBB);
  m_face.listen(m_clientPrefix, bind(&Client::processIncomingInterest, this, _2));
}

void
Client::startOp(const NfsOp& op)
{
  switch (op.proc) {
  case NFS_GETATTR:
  case NFS_LOOKUP:
    this->sendCommand(op, {name::Component("."), name::Component("attr")},
                      ServerAction{SA_ATTR, op.version, 0}, false);
    break;
  case NFS_READLINK:
    this->sendCommand(op, {},
                      ServerAction{SA_READLINK, op.version, 0}, false);
    break;
  case NFS_READ:
    this->startRead(op);
    break;
  case NFS_WRITE:
    this->startWrite(op);
    break;
  case NFS_READDIRP:
    this->startReadDir(op);
    break;
  case NFS_SETATTR:
  case NFS_CREATE:
  case NFS_MKDIR:
  case NFS_SYMLINK:
  case NFS_REMOVE:
  case NFS_RMDIR:
  case NFS_RENAME:
    this->sendCommand(op, {name::Component("."), name::Component(NfsProcStrings[op.proc])},
                      ServerAction{SA_SIMPLECMD, 0, 0}, false);
    break;
  case NFS_ACCESS: // ignored
  default:
    break;
  }
}

void
Client::periodicalCleanup()
{
  this->expireWrites();
}

Interest
Client::makeCommand(const std::string& path, const std::vector<name::Component>& appendToName,
                    const ServerAction& sa, bool needSignature)
{
  Name name(m_serverPrefix);
  name.append(Name(path));
  for (const name::Component& comp : appendToName) {
    name.append(comp);
  }

  if (needSignature) {
    appendSignature(name);
  }

  Interest interest(name);
  interest.setExclude(sa);
  interest.setMustBeFresh(true);
  return interest;
}

void
Client::sendCommand(const NfsOp& op, const std::vector<name::Component>& appendToName,
                    const ServerAction& sa, bool needSignature)
{
  Interest interest = this->makeCommand(op.path, appendToName, sa, needSignature);

  EmulationTime start = EmulationClock::now();
  requestAutoRetry(m_face, interest,
                   bind([=] { this->opSuccess(op, start, EmulationClock::now()); }),
                   bind([=] { this->opFailure(op, start, EmulationClock::now()); }),
                   AutoRetryLimited(AUTO_RETRY_LIMIT));
}

void
Client::processIncomingInterest(const Interest& interest)
{
  ServerAction sa = ServerAction::fromExclude(interest.getExclude());
  switch (sa.verb) {
  case SA_FETCH:
    this->processFetchInterest(interest, sa);
    break;
  default:
    break;
  }
}

void
Client::startRead(const NfsOp& op)
{
  Name name(m_serverPrefix);
  name.append(Name(op.path));
  name.appendVersion(op.version);

  EmulationTime start = EmulationClock::now();
  requestSegments(m_face, name, {op.segStart, op.segStart + op.nSegments - 1},
                  nullptr,
                  bind([=] { this->opSuccess(op, start, EmulationClock::now()); }),
                  bind([=] { this->opFailure(op, start, EmulationClock::now()); }),
                  AutoRetryLimited(AUTO_RETRY_LIMIT),
                  [] (Interest& interest) {
                    interest.setExclude(ServerAction{SA_READ, 0, SEGMENT_SIZE});
                  });
}

void
Client::startReadDir(const NfsOp& op)
{
  Name name(m_serverPrefix);
  name.append(Name(op.path));
  name.append(name::Component("."));
  name.append(name::Component("dir"));
  name.appendVersion(op.version);

  EmulationTime start = EmulationClock::now();
  requestSegments(m_face, name, {0, op.nSegments},
                  nullptr,
                  bind([=] { this->opSuccess(op, start, EmulationClock::now()); }),
                  bind([=] { this->opFailure(op, start, EmulationClock::now()); }),
                  AutoRetryLimited(AUTO_RETRY_LIMIT),
                  [&] (Interest& interest) {
                    const Name& interestName = interest.getName();
                    if (interestName.at(-1).toSegment() == 0) {
                      interest.setName(interestName.getPrefix(-2));
                      interest.setExclude(ServerAction{SA_READDIR1, op.version, DIR_PER_SEGMENT});
                      interest.setMustBeFresh(true);
                    }
                    else {
                      interest.setExclude(ServerAction{SA_READDIR2, 0, DIR_PER_SEGMENT});
                    }
                  });
}

void
Client::startWrite(const NfsOp& op)
{
  Name fetchPrefix(m_clientPrefix);
  fetchPrefix.append(Name(op.path));
  fetchPrefix.appendVersion(op.version);
  WriteProcess& wp = m_writes[fetchPrefix];
  wp.start = EmulationClock::now();
  wp.op = op;
  wp.lastFetch = EmulationTime::max();

  std::stringstream params;
  params << m_clientHost << ':' << op.version << ':'
         << op.segStart << ':' << (op.segStart + op.nSegments - 1);
  Interest writeCmd = this->makeCommand(op.path,
      {name::Component("."), name::Component("write"), name::Component(params.str())},
      ServerAction{SA_WRITE, op.version, 0}, true);

  requestAutoRetry(m_face, writeCmd,
                   bind([this, fetchPrefix] {
                     m_writes[fetchPrefix].lastFetch = EmulationClock::now();
                   }),
                   bind([this, fetchPrefix] {
                     WriteProcess& wp = m_writes[fetchPrefix];
                     this->opFailure(wp.op, wp.start, EmulationClock::now());
                     m_writes.erase(fetchPrefix);
                   }),
                   AutoRetryLimited(AUTO_RETRY_LIMIT));
}

void
Client::processFetchInterest(const Interest& interest, const ServerAction& sa)
{
  Name fetchPrefix = interest.getName().getPrefix(-1);
  auto it = m_writes.find(fetchPrefix);
  if (it == m_writes.end()) {
    // not a WRITE in progress
    // TODO nack
    return;
  }

  WriteProcess& wp = it->second;
  wp.lastFetch = EmulationClock::now();
  wp.fetchedSegments.insert(interest.getName().at(-1).toSegment());

  Data data(interest.getName());
  data.setContent(m_payloadBuffer, SEGMENT_SIZE);
  m_face.reply(data);

  if (wp.fetchedSegments.size() == wp.op.nSegments) {
    this->finishWrite(fetchPrefix);
  }
}

void
Client::finishWrite(const Name& fetchPrefix)
{
  WriteProcess& wp = m_writes.at(fetchPrefix);
  const NfsOp& op = wp.op;

  std::stringstream params;
  params << m_clientHost << ':' << op.version << ':'
         << op.segStart << ':' << (op.segStart + op.nSegments - 1);
  Interest commitCmd = this->makeCommand(op.path,
      {name::Component("."), name::Component("commit"), name::Component(params.str())},
      ServerAction{SA_COMMIT, op.version, 0}, true);

  requestAutoRetry(m_face, commitCmd,
                   bind([this, fetchPrefix] {
                     auto it = m_writes.find(fetchPrefix);
                     BOOST_ASSERT(it != m_writes.end());
                     WriteProcess& wp = it->second;
                     this->opSuccess(wp.op, wp.start, EmulationClock::now());
                     m_writes.erase(it);
                   }),
                   bind([this, fetchPrefix] {
                     auto it = m_writes.find(fetchPrefix);
                     BOOST_ASSERT(it != m_writes.end());
                     WriteProcess& wp = it->second;
                     this->opFailure(wp.op, wp.start, EmulationClock::now());
                     m_writes.erase(it);
                   }),
                   AutoRetryLimited(AUTO_RETRY_LIMIT));
}

void
Client::expireWrites()
{
  EmulationTime minLastFetch = EmulationClock::now() - FETCH_MAX_GAP;
  for (auto it = m_writes.begin(); it != m_writes.end();) {
    if (it->second.lastFetch < minLastFetch) {
      it = m_writes.erase(it);
    }
    else {
      ++it;
    }
  }
}

/** \brief drives an emulation session
 */
class EmulationRunner
{
public:
  EmulationRunner(OpsParser& trace, Client& client,
                  boost::asio::io_service& io, std::ostream& log);

  /** \brief start replaying the trace
   *  \param now synchronize NfsTimestamp now with EmulationClock::now()
   *  \param timeFactor >1 replays faster, <1 replays slower
   */
  void
  start(NfsTimestamp now, float timeFactor = 1.0);

  EventEmitter<> onFinish;

private:
  EmulationTime
  computeEmulationTime(NfsTimestamp nfsTime) const;

  void
  run();

  /** \brief no more operations
   */
  void
  finish();

  void
  periodicalCleanupThenReschedule();

private:
  boost::asio::io_service& m_io;
  OpsParser& m_trace;
  Client& m_client;
  std::ostream& m_log;

  Scheduler m_scheduler;
  EventId m_periodicalCleanup;
  static const EmulationClock::Duration CLEANUP_INTERVAL;
  static const EmulationClock::Duration WAIT_AFTER_LAST_OP;

  bool m_isStarted;
  EmulationTime m_startEmulationTime;
  NfsTimestamp m_startNfsTimestamp;
  double m_timeFactor;

  NfsOp m_nextOp;

};
const EmulationClock::Duration EmulationRunner::CLEANUP_INTERVAL = time::seconds(10);
const EmulationClock::Duration EmulationRunner::WAIT_AFTER_LAST_OP = time::seconds(20);

EmulationRunner::EmulationRunner(OpsParser& trace, Client& client,
                                 boost::asio::io_service& io, std::ostream& log)
  : m_io(io)
  , m_trace(trace)
  , m_client(client)
  , m_log(log)
  , m_scheduler(io)
  , m_isStarted(false)
{
  m_client.opSuccess += [this] (const NfsOp& op,
                                const EmulationTime& start, const EmulationTime& end) {
    m_log << op << ','
          << "SUCCESS" << ','
          << time::duration_cast<time::microseconds>(start.time_since_epoch()).count() << ','
          << time::duration_cast<time::microseconds>(end.time_since_epoch()).count() << ','
          << time::duration_cast<time::microseconds>(end - start).count() << std::endl;
  };
  m_client.opFailure += [this] (const NfsOp& op,
                                const EmulationTime& start, const EmulationTime& end) {
    m_log << op << ','
          << "FAILURE" << ','
          << time::duration_cast<time::microseconds>(start.time_since_epoch()).count() << ','
          << time::duration_cast<time::microseconds>(end.time_since_epoch()).count() << ','
          << time::duration_cast<time::microseconds>(end - start).count() << std::endl;
  };
}

void
EmulationRunner::start(NfsTimestamp now, float timeFactor)
{
  BOOST_ASSERT(!m_isStarted);
  m_isStarted = true;

  m_startEmulationTime = EmulationClock::now();
  m_startNfsTimestamp = now;
  m_timeFactor = timeFactor;

  this->periodicalCleanupThenReschedule();

  this->run();
}

EmulationTime
EmulationRunner::computeEmulationTime(NfsTimestamp nfsTime) const
{
  double microsecondsFromStart = (nfsTime - m_startNfsTimestamp) / m_timeFactor;
  return m_startEmulationTime +
         time::microseconds(static_cast<time::microseconds::rep>(microsecondsFromStart));
}

void
EmulationRunner::run()
{
  m_nextOp = m_trace.read();
  if (m_nextOp.proc == NFS_NONE) {
    this->finish();
  }

  EmulationClock::Duration slack = this->computeEmulationTime(m_nextOp.timestamp) -
                                   EmulationClock::now();
  m_scheduler.scheduleEvent(slack, [this] {
    m_client.startOp(m_nextOp);
    this->run();
  });
}

void
EmulationRunner::finish()
{
  m_scheduler.cancelEvent(m_periodicalCleanup);
  m_scheduler.scheduleEvent(WAIT_AFTER_LAST_OP, [this] {
    m_client.periodicalCleanup();
    onFinish();
  });
}

void
EmulationRunner::periodicalCleanupThenReschedule()
{
  m_client.periodicalCleanup();
  m_periodicalCleanup = m_scheduler.scheduleEvent(CLEANUP_INTERVAL,
      bind(&EmulationRunner::periodicalCleanupThenReschedule, this));
}

int
client_main(int argc, char* argv[])
{
  boost::asio::io_service io;
  NackEnabledFace face(io);
  OpsParser trace(std::cin);
  Client client(face, "ndn:/NFS", std::string("ndn:/") + argv[2]);

  EmulationRunner runner(trace, client, io, std::cout);
  runner.onFinish += [&] {
    io.stop();
  };
  runner.start(boost::lexical_cast<uint64_t>(argv[1]));

  io.run();
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