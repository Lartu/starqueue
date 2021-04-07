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
