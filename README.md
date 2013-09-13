simpletsdb-client 0.1
=====================

This is a dead-simple C++ telnet-style client for OpenTSDB, using Boost.Asio.

The client is self-contained in a single class (see SimpleTSDBClient.hh). Typical
usage is as follows

    SimpleTSDBClient client(host, port);

    // call client.run() in a separate thread
    std::thread t(boost::bind(&SimpleTSDBClient::run, &client));

    // feed some data points to the tsd
    SimpleTSDBClient::TagsType tags;
    tags["foo"] = "bar";
    tags["bar"] = "baz";
    client.add_point("my_metric", timestamp, 123.456, &tags);

    // close the client and wait for worker thread
    client.close();
    t.join();

Building
========

A sample program (`test_client.cc`) is included, plus a Makefile demonstrating
how to compile.

SimpleTSDBClient relies on Boost.Asio to build. All required Boost libraries
are included in the `third_party` directory, but note that Boost.Asio depends 
on Boost.System library, which is not header-only. You have two options for
integrating SimpleTSDBClient with your code:

 1. cd into `third_party/boost` and run `./bjam` to build the Boost libraries.
    You can then link against them directly, and distribute as you like
 2. Include `third_party/boost/libs/system/src/error_code.cpp` in your
    compilation (this is what the included Makefile does)

Alternatively, you can use your OS's Boost distribution. On Ubuntu:

    sudo apt-get install libboost-dev

Notes on included libraries
===========================

Included are full copies of Boost.Asio and its dependencies from Boost 1.53.0.
Note that Boost 1.54.0 has a bug in Asio (see https://svn.boost.org/trac/boost/ticket/8795)
which is the reason for including the older build of Asio.


