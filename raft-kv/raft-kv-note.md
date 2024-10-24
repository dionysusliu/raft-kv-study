## Questions

#### What is `RaftNode` for?
- What resource does it abstract
- What functions does it delegate
- What interfaces does it provide?
- What modules would utilize this class?

#### What is `Transport` module for?
Looks like IPC layer?
- what functions does it delegate?
- what interfaces does it provide?
- how to use this class?


## Takeaways
#### process signal capture
```c++
void on_signal(int) {
  LOG_INFO("catch signal");
  if (g_node) {
    g_node->stop();
  }
}
...
::signal(SIGINT, on_signal);
::signal(SIGHUP, on_signal);
```
`::signal` means use the global C standard library `signal()`, it register a callback function when some signal is catched by process

`SIGINT` means process is *interrupted by user*

`SIGHUP` means the *controlling terminal is closed / disconnected*

`on_signal` is the callback here. it must be a function of type `void(*)(int)`, where the argument is the signal code