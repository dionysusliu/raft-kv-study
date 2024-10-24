#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <raft-kv/transport/transport.h>
#include <raft-kv/common/log.h>

namespace kv {

class TransportImpl : public Transport {

 public:
  explicit TransportImpl(RaftServer* raft, uint64_t id)
      : raft_(raft),
        id_(id) {
  }

  ~TransportImpl() final {
    if (io_thread_.joinable()) {
      io_thread_.join();
      LOG_DEBUG("transport stopped");
    }
  }

  void start(const std::string& host) final {
    server_ = IoServer::create((void*) &io_service_, host, raft_);
    server_->start(); // setup callback on tcp connection

    io_thread_ = std::thread([this]() {
      this->io_service_.run(); // start event loop in new thread
    });
  }

  void add_peer(uint64_t id, const std::string& peer) final {
    LOG_DEBUG("node:%lu, peer:%lu, addr:%s", id_, id, peer.c_str());
    std::lock_guard<std::mutex> guard(mutex_);

    auto it = peers_.find(id);
    if (it != peers_.end()) {
      LOG_DEBUG("peer already exists %lu", id);
      return;
    }

    PeerPtr p = Peer::creat(id, peer, (void*) &io_service_);
    p->start();
    peers_[id] = p;
  }

  void remove_peer(uint64_t id) final {
    LOG_WARN("no impl yet");
  }

  void send(std::vector<proto::MessagePtr> msgs) final {
    // new event: send the message to all peers in cluster
    auto callback = [this](std::vector<proto::MessagePtr> msgs) {
      for (proto::MessagePtr& msg : msgs) {
        if (msg->to == 0) {
          // ignore intentionally dropped message
          continue;
        }

        auto it = peers_.find(msg->to);
        if (it != peers_.end()) {
          it->second->send(msg);
          continue;
        }
        LOG_DEBUG("ignored message %d (sent to unknown peer %lu)", msg->type, msg->to);
      }
    };
    io_service_.post(std::bind(callback, std::move(msgs))); // post the event
  }

  void stop() final {
    io_service_.stop();
  }

 private:
  // Server-role logics
  // accept connection and receive message from other nodes
  // parse message and forward to Raft Algorithm and Storage logics
  RaftServer* raft_;
  uint64_t id_;

  // event loop for cluster network events
  // shared by
  // (1) transport [general events dispatching]
  // (2) IO server (server-role events)
  // (3) peers (client-role events)
  std::thread io_thread_;
  boost::asio::io_service io_service_;

  // Client-role logics
  // each peer is a proxy of remote node, through which we send message
  std::mutex mutex_;
  std::unordered_map<uint64_t, PeerPtr> peers_;

  IoServerPtr server_;
};

std::shared_ptr<Transport> Transport::create(RaftServer* raft, uint64_t id) {
  std::shared_ptr<TransportImpl> impl(new TransportImpl(raft, id));
  return impl;
}

}
