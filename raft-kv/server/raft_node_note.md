## Questions
#### 1. What is 'Node' for
- Its differences from `RaftNode`
- What functions does it delegate?
- What interfaces does it provide?
- How to use it?

#### 2. What are differences between `start_node` and `restart_node`

#### 3. What does replay_WAL do?
- Seems like it recover a lot of in-memory data structures from snapshot
  - what are they, how are they represented

#### 4. What is `redis_server_` for

#### what is `io_service` here?
- what OS resources / interfaces does it abstract
- what interfaces does it provide to developers
- what design pattern does it apply?

#### what is `timer` for? (answered)

### what are difference between `RaftNode` and `IoServer`
RaftNode is NOT a `IoServer` instance, so how does it communicate with networking layer?
If it would be registered into some `IoServer` instance, then where? I don't see the init of `IoServer` in main()?
- is it in the `node_->schedule()` logic? 



## Takeaways
#### async programming via promise/future 
```c++
redis_server_ = std::make_shared<RedisStore>(this, std::move(snap_data_), port_);
std::promise<pthread_t> promise;
std::future<pthread_t> future = promise.get_future();
redis_server_->start(promise); // seems like an async wait
future.wait();
pthread_t id = future.get();
LOG_DEBUG("server start [%lu]", id);
```
`promise` and `future` are common primitives for async programming. 
- `promise` holds a thread ID which hasn't been fulfilled yet
- `promise.get_future()` returns a `future` object, which represents the result of some computation
  - In this case, it is the thread_id
  - The `future` object allows the program to wait for results **without blocking**.
- `redis_server_->start()` takes a promise as argument
  - create async working threads / tasks, and return immediately
  - the promise value is set in the working thread
- `future.wait()` blocks this thread, until the redis server is started and the `promise` is set
- `id = future.get()` would get id of that separate working thread for redis IO

#### async periodic task via timer 
```c++
void RaftNode::start_timer() {
  timer_.expires_from_now(boost::posix_time::millisec(100));
  timer_.async_wait([this](const boost::system::error_code& err) {
    if (err) {
      LOG_ERROR("timer waiter error %s", err.message().c_str());
      return;
    }

    this->start_timer();
    this->node_->tick();
    this->pull_ready_events();
  });
}
```
This code setup a periodic task that is executed per 100ms. 
- `expires_from_now()` resets period the timer waits to **fire**
- `async_wait(func)` register the function that timer should execute on firing