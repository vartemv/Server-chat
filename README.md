# IPK - second project - Server-chat

This is a simple chat application written in C++.

**Table of contents:**

- [IPK - first project - Client-chat](#ipk---first-project---client-chat)
    - [Theory](#theory)
    - [Implementation](#implementation)
        - [UDP communication](#udp-communication)
            - [Message confirmation tracking](#message-confirmation-tracking)
        - [TCP communication](#tcp-communication)
    - [Testing](#testing)
        - [UDP tests](#udp-tests)
        - [TCP tests](#tcp-tests)

# Theory

- ### TCP

  Transmission Control Protocol is a transport protocol that is used on top of IP (Internet Protocol) to ensure reliable
  transmission of packets over the internet or other networks. TCP is a connection-oriented protocol, which means that
  it establishes and maintains a connection between the two parties until the data transfer is complete. TCP provides
  mechanisms to solve problems that arise from packet-based messaging, e.g. lost packets or out-of-order packets,
  duplicate packets, and corrupted packets. TCP achieves this by using sequence and acknowledgement numbers, checksums,
  flow control, error control, and congestion control.

- ### UDP

  User Datagram Protocol is a connectionless and unreliable protocol that provides a simple and efficient way to send
  and receive datagrams over an IP network. UDP does not guarantee delivery, order, or integrity of the data, but it
  minimizes the overhead and latency involved in transmitting data when compared to TCP. UDP is suitable for
  applications that require speed, simplicity, or real-time communication, such as streaming media, online gaming, voice
  over IP, or DNS queries.

- ### Thread Pool

  Thread pool is a software design pattern for achieving concurrency of execution in a computer program. Essentially, a
  thread pool is a group of pre-instantiated, idle threads which stand ready to be given work. These are preferred over
  instantiating new threads for each task when there is a large number of short tasks to be done rather than a small
  number of long ones. This prevents having to incur the overhead of creating a thread a large number of times.

***

# Implementation
##### More detailed graphs can be found in doxy_doc generated using Doxygen on my repository

