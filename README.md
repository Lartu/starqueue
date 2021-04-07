<p align="center">
  <img src="https://github.com/Lartu/starqueue/blob/main/starqueue.png">
  <br><br>
  <img src="https://img.shields.io/badge/dev._version-v1.0.0-blue.svg">
  <img src="https://img.shields.io/badge/license-BSD_2.0-purple">
  <img src="https://img.shields.io/badge/starfish-many-yellow">
  <img src="https://img.shields.io/badge/prod._ready-almost_there-orange">
</p>

# StarQueue
An extremely simple message queue server for distributed applications and microservices, **StarQueue** has been designed with simplicity in mind. It works as a remote FIFO queue that can perform insertion and retrieval of string values. It's extremely fast and has been designed from the ground up to be used with distributed applications and microservices. For long term storage, cyclical *checkpoint* saves are executed.

# Building and installation
* To **build** StarQueue, clone this repository and run `make` in it. C++11 is required.
* To **install** StarQueue, run `make install`.
* To **uninstall** StarQueue, simply run `make uninstall`. You will also manually have to delete any checkpoint files you may have created while using StarQueue.

# Usage
StarQueue can be used both as a local or a remote message queue. In both cases, running `starqueue` starts the StarQueue server 
on the default StarQueue port (17827) using the file `starqueue.qu` on your home directory as its checkpoint file. Check `starqueue --help` for
information on how to launch StarQueue with a custom configuration.

Programs can connect to StarQueue to access the message queue system using its port and IP address (`localhost` for local systems). To communicate with
StarQueue, 5 commands can be used, sent via a normal TCP socket stream. Sent messages **must** be terminated using the `\r\n` character sequence.
Messages sent by CoralDB always end with said character sequence, too.

A client must establish a new connection to StarQueue every time it wants to send a command.
Likewise, StarQueue closes the connection after a response has been sent to the client.

## Command Index
Command | Description
:---: | :---:
[ENQUEUE](#ENQUEUE) | Enqueues a message into the queue.
[DEQUEUE](#DEQUEUE) | Dequeues and retrieves the next message from the queue.
[SIZE](#SIZE) | Returns the size of the queue.
[CLEAR](#CLEAR) | Empties que queue.
[PING](#PING) | Checks if the database is online.
[CHECKPOINT](#CHECKPOINT) | Forces a queue checkpoint save.

### ENQUEUE

**Usage**: ```ENQUEUE "value"```

**ENQUEUE** enqueues `"value"` into the queue. Values must be enclosed in double-quotes (`"`). Double-quotes inside a value may be escaped using the `\"` character sequence.
* Responses:
  * `OK.` for successful operations.
  * `ERROR.` for unsuccessful operations.

### DEQUEUE

**Usage**: ```DEQUEUE```

**DEQUEUE** gets the next value from the queue. The operation fails if the queue is empty.
* Responses:
  * `"value"` for successful operations, where `value` is the value retrieved. Note that the value is enclosed in double-quotes.
  * `ERROR.` for unsuccessful operations.


### SIZE

**Usage**: ```SIZE```

**SIZE** returns the size of the queue as an integer.
* Responses:
  * The queue size (for example, `1293`).

### CLEAR

**Usage**: ```CLEAR```

**CLEAR** empties the queue.
* Responses:
  * `OK.`

### PING

**Usage**: ```PING```

**PING** can be used to check if StarQueue is alive.
* Responses:
  * `OK.` for successful operations.

### CHECKPOINT

**Usage**: ```CHECKPOINT```

**CHECKPOINT** can be used to trigger a checkpoint save.
* Responses:
  * `OK.` once the checkpoint save has finished.

# Example Session

In this example, `\r\n` at the end of lines are omitted.

```
Client:    ENQUEUE "hi there"
StarQueue: OK.
Client:    ENQUEUE "gotta go!"
StarQueue: OK.
Client:    ENQUEUE "bye!"
StarQueue: OK.
Client:    DEQUEUE
StarQueue: "hi there"
Client:    SIZE
StarQueue: 2
Client:    DEQUEUE
StarQueue: "gotta go!"
Client:    CLEAR
StarQueue: OK.
Client:    DEQUEUE
StarQueue: ERROR.
```

# I need help! I want to contribute!

If you need help on using StarQueue, please open an issue here and I will try to address it as soon as possible.

Contributions are also more than welcome! Feel free to submit pull requests!

# License

StarQueue is released under the [BSD 2-Clause License](https://github.com/Lartu/coraldb/blob/main/LICENSE).
