# ROS 2 Fundamentals

## Basic Buidling Blocks
1) nodes
    - process that performs some task
    - implemented as subclass that inherites from ROS node class
    - requires oops' encapsulation and abstraction
    - each nodes run in their own runtime environement
    - each node is written in its own file such as python and cpp
    - can be written in almost any programming language

3) Messages
    - communication "language" between nodes
    - contain 1 or more fields with strongly-typed data such as string int floating point arrays etc
    - each Message must be defined by an interface

4) Interface
    - format of messsage
    - descrption of what kind of data the message should contain
    - programming api which defines what kind of data should be recieved or sent by a node either in a topic or a service
    - ROS comes with a number of pre-defined interfaces, you can also define your own
    - nodes can instantiate 1 or more of these messaging types (interfaces)
    - nodes can have more that 1 publishers, subscribers, clients and servers



2) Method of Communication between nodes:
    (a) Topics
    - named communication channel for publish-subscribe model
    - nodes can subscribe to 1 or more topics and when a message is published to that topic, all subscribing nodes will recieve it

    (b) Service (Synchronous Communication method)
    - Client will send a request to a server
    - Server is expected to reply with a response
    - Requests-Response are specific type of messages