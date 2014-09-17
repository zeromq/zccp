# ZCCP - ZeroMQ Command &amp; Control Protocol

ZCCP is an system designed for monitoring and controlling a large network of devices. An example use case is monitoring of a data center for power consumption, network congestion, virtual machine overload, and so on.

ZCCP does three main things:

* It collects notifications from a set of producers, and it relays these notifications to a set of consumers.
* It routes control commands, and their responses, to and from peers in the network.
* It provides easy integration of user application code into this network.

ZCCP is a broker-based design, where all traffic flows through a single broker which manages subscriptions and request-reply delivery.

In the typical use case, we have device drivers hand-written for various kinds of hardware. These drivers use the ZCCP protocol to talk to the broker. We then have client applications that receive these notifications, selectively. Finally, these applications can take action by sending control commands to individual devices, or to groups of devices.

ZCCP is language and payload neutral, so device drivers and applications can be written in any language.

Features:

* Includes client APIs in C, Java, and Go.
* ZCCP broker is written in C, using ZeroMQ zproto framework.

## Building

First, install libzmq and CZMQ from GitHub.

Then, build the broker and shell client:

    gcc -g -o zccpd zccpd.c zccp_msg.c zccp_server.c -lczmq -lzmq
    gcc -g -o zccp_sh zccp_sh.c zccp_msg.c zccp_server.c zccp_client.c -lczmq -lzmq

## Known Problems

* Project needs real packaging (makefiles, etc.) to build libzccp.

* Subscription implementation in zccp_server is not fast, nor compact. It will also send messages multiple times, if clients have multiple matching subscriptions.

* Request-reply message flow has not been developed yet. It could be implemented today using the pub-sub mechanisms.
