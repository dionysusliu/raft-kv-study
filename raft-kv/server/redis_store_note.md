
# questions 

#### What is `io_service_`? (answered)
#### What is `acceptor_`? (answered)
#### What does `RaftNode::propose()` do?
- there is this line at the end of `RedisStore::set()`:
```
server_->propose(
   std::move(data), 
   some_custom_callback
)
```
- So it takes some uniquely-owned data and a callback? What is it trying to do? 
- Looks like some kind of async stuff, but what exactly?

#### What is `start_accept()` for?  (answered)

#### What happens to each set/del/get operation?
    
- When I type `set key1 val1` in redis-cli:
  - what message is sent to the server?
  - where in the code does our KV receive this message?
  - where does it parse the message?
  - where does it execute the message?
  - where is the single-point key-val update broadcast in the cluster
  - where does it do the post-execution logics?


## Takeaways
### async programming with promise/future
```c++
void RedisStore::start(std::promise<pthread_t>& promise) {
  start_accept();

  worker_ = std::thread([this, &promise]() {
    promise.set_value(pthread_self());
    this->io_service_.run();
  });
}
```
`start()` dispatch a new thread for IO services. It also set the promise in this thread, in order to notify another thread with `future`

### How Boost abstract "async event handling" with `io_service`
- what kind of "service" does it provide?
  - It serves as an "event dispatcher" that can handle multiple concurrent async IO events ("conn / w / r / signal" of  "networking/fileIO/timer/...")
- what does it abstract? the OS? or something above?
  - eventloop, callback on event, and thread-pool for concurrent events

### `RedisSession` is a good example of "decoupling" IO from CPU works
The design and usage of `RedisSession` reflects many useful design patterns:
1. **Factory**: each `start_accept()` creates a new RedisSession. Semantically, we establish a new redis session on each new connection, so the design fits the need well.
2. **Proxy**: provide interface for control of (1) net socket & (2) redis protocol parsing.
3. **Publisher**: when RedisSession (publisher) receives a new request and validate it, it would notify the upper RedisStore (observer) by calling their handler (get, set and del).
    ```c++
      RedisSessionPtr session(new RedisSession(this, io_service_));
    ```
  - notice that `this` is included in the constructor of RedisSession. This is a `shared_ptr` or `weak_ptr`
  - `RedisStore` provides a set of interfaces for sessions to access in-memory storage: `set`, `del`, `get`. Like a "micro-protocol" between sub-modules 
4. **Dependency Injection**: this is from boost design. The functionality of `acceptor` and `socket` handling concurrent IO events comes from implementation of `io_service` object. 
   - by definition of constructor, they take a "executor". This means they need a object that "have" some functions. these functions are implemented by `io_service` class and *injected* into `acceptor`/`socket` via a `io_service` instance
   ```c++ 
   auto socket_ = boost::asio::ip::tcp::socket(io_service);
   ```
With all these design patterns, we beautifully **decouple the storage logics and the network IO logics**. 
`RedisStore` focuses on the data storage and retrieval, while `RedisSession` deals with incoming `redis-cli` requests. And they can communicate to each other via public handlers. This decoupling of functionality is known as **Single Responsibility Principle**.

Decoupling **storage engine** and **IO logics** is a best practice, since:
- We have *different storage engine*: unordered_map, leveldb, rocksdb, ...
- We have *different communication protocol*: redis, mysql, postgres | or even tcp, udp, ...
Decoupling them with shared inter-module interfaces makes the software easier to scale and support multiple implementations.