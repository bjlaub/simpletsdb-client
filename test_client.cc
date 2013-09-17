#include <iostream>
#include <thread>
#include <cstdlib>
#include <boost/asio.hpp>

#include "SimpleTSDBClient.hh"

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: chat_client <host> <port>\n";
            return 1;
        }
        simpletsdb::SimpleTSDBClient client(argv[1], atoi(argv[2]));

        // start the client
        std::thread t(boost::bind(&simpletsdb::SimpleTSDBClient::run, &client));

        // feed some data points
        simpletsdb::SimpleTSDBClient::TagsType tags;
        tags["foo"] = "bar";
        for(int i = 0; i < 30; i++)
        {
            client.add_point("testmetric", i, 555, &tags);
            sleep(1);
        }

        client.close();
        t.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

