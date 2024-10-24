
# Takeaways
#### Segregate "interface sets" for one server instance
The server logics of a Raft node consists of multiple modules. The implementation here isolation the interfaces for 
different modules. In this case, difference interface classes are **loosely coupled** and we can feel free to change 
implementation for one modules with affecting codebase of the other one.
```c++
 // * defines how one raft node interact with the Raft Cluster
class RaftServer {
 public:
  virtual ~RaftServer() = default;

  virtual void process(proto::MessagePtr msg, const std::function<void(const Status&)>& callback) = 0;

  virtual void is_id_removed(uint64_t id, const std::function<void(bool)>& callback) = 0;

  virtual void report_unreachable(uint64_t id) = 0;

  virtual void report_snapshot(uint64_t id, SnapshotStatus status) = 0;

  virtual uint64_t node_id() const = 0;
};

// connection layer of a RaftServer
// handle network connection and events for RaftServer
class IoServer {
 public:
  virtual void start() = 0;
  virtual void stop() = 0;

  static std::shared_ptr<IoServer> create(void* io_service,
                                          const std::string& host,
                                          RaftServer* raft);
};
```
Here, we have two interfaces: `IoServer` and `RaftServer`. 
The `IoServer` is the top modules we use, which *combines* a `RaftServer` instance and an `io_services`. In this 
case, (1) `io_service` would handle the logic of communication and IO event loop (2) the top `IoServer` would *forward*
incoming messages from clusters to the raft server (3) `RaftServer` would care about raft node logics and handling 
incoming messages, *without worrying about how to receive and dispatch messages*.