#ifndef __SIMPLE_TSDB_CLIENT_HH__
#define __SIMPLE_TSDB_CLIENT_HH__

#include <string>
#include <deque>
#include <map>
#include <string>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

namespace simpletsdb {

using boost::asio::ip::tcp;

class SimpleTSDBClient
{
public:
    typedef std::map<std::string, std::string> TagsType;

    SimpleTSDBClient(const std::string & host, unsigned short port)
        : m_io_service(),
          m_connected(false),
          m_host(host),
          m_port(port),
          m_reconnect_timer(m_io_service),
          m_resolver(m_io_service)
    {
        start_connect();
    }

    // run() should be called from a separate thread
    void run()
    {
        m_work.reset(new boost::asio::io_service::work(m_io_service));
        m_io_service.run();
    }

    template <typename T>
    void add_point(
        const std::string & metric,
        double timestamp,
        const T & value,
        const TagsType * const ptags)
    {
        write(format(metric, timestamp, value, ptags));
    }

    void close()
    {
        m_work.reset();
        m_io_service.post(boost::bind(&SimpleTSDBClient::do_close, this));
    }

private:
    void write(const std::string & data)
    {
        m_io_service.post(boost::bind(&SimpleTSDBClient::do_write, this, data));
    }

    template <typename T>
    std::string format(
        const std::string & metric,
        double timestamp,
        const T & value,
        const TagsType * const ptags)
    {
        std::ostringstream oss;
        long ts = static_cast<long>(timestamp);
        oss << "put "
            << metric << " "
            << ts << " "
            << value << " ";

        if(ptags)
        {
            const TagsType & tags = *ptags;
            for(TagsType::const_iterator it = tags.begin();
                it != tags.end();
                ++it)
            {
                oss << it->first << "=" << it->second << " ";
            }
        }

        oss << "\n";
        return oss.str();
    }

    void start_connect()
    {
        tcp::resolver::query query(m_host, boost::lexical_cast<std::string>(m_port));
        m_resolver.async_resolve(query,
            boost::bind(&SimpleTSDBClient::handle_resolve, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::iterator));
    }

    void restart_connect(const boost::system::error_code & error)
    {
        if(!error)
        {
            // not interrupted during an async_wait
            start_connect();
        }
        else
        {
            std::cout << "restart_connect was interrupted!\n";
            // TODO: ?
        }
    }

    void handle_resolve(
        const boost::system::error_code & error,
        tcp::resolver::iterator ep_it)
    {
        if(!error)
        {
            // attempt to connect to each endpoint in iterator until we fail
            m_socket.reset(new tcp::socket(m_io_service));
            boost::asio::async_connect(*m_socket, ep_it,
                boost::bind(&SimpleTSDBClient::handle_connect, this,
                    boost::asio::placeholders::error));
        }
        else
        {
            // restart in 2 seconds
            m_reconnect_timer.expires_from_now(boost::posix_time::seconds(2));
            m_reconnect_timer.async_wait(
                boost::bind(&SimpleTSDBClient::restart_connect, this,
                    boost::asio::placeholders::error));
        }
    }

    void handle_connect(const boost::system::error_code & error)
    {
        if(!error)
        {
            std::cout << "connected!\n";
            m_connected = true;

            if(!m_pending.empty())
            {
                // send first message from the queue now
                m_socket->async_send(
                    boost::asio::buffer(m_pending.front().data(), m_pending.front().length()),
                    boost::bind(&SimpleTSDBClient::handle_write, this, boost::asio::placeholders::error));
            }
        }
        else
        {
            std::cout << "not connected...\n";
            m_reconnect_timer.expires_from_now(boost::posix_time::seconds(2));
            m_reconnect_timer.async_wait(
                boost::bind(&SimpleTSDBClient::restart_connect, this,
                    boost::asio::placeholders::error));
        }
    }

    void handle_write(const boost::system::error_code & error)
    {
        if(!error)
        {
            m_pending.pop_front();
            if(!m_pending.empty())
            {
                m_socket->async_send(
                    boost::asio::buffer(m_pending.front().data(), m_pending.front().length()),
                    boost::bind(&SimpleTSDBClient::handle_write, this, boost::asio::placeholders::error));
            }
        }
        else
        {
            std::cout << "write failed: " << error << "\n";
            m_socket.reset();
            m_connected = false;
            m_reconnect_timer.expires_from_now(boost::posix_time::seconds(2));
            m_reconnect_timer.async_wait(
                boost::bind(&SimpleTSDBClient::restart_connect, this,
                    boost::asio::placeholders::error));
        }
    }

    void do_write(std::string data)
    {
        bool write_in_progress = !m_pending.empty();
        m_pending.push_back(data);

        if(!m_connected) return;

        if(!write_in_progress)
        {
            m_socket->async_send(
                boost::asio::buffer(m_pending.front().data(), m_pending.front().length()),
                boost::bind(&SimpleTSDBClient::handle_write, this, boost::asio::placeholders::error));
        }
    }

    void do_close()
    {
        m_socket->close();
    }

private:
    boost::asio::io_service m_io_service;
    boost::shared_ptr<tcp::socket> m_socket;
    std::deque<std::string> m_pending;
    bool m_connected;
    std::string m_host;
    unsigned short m_port;
    boost::asio::deadline_timer m_reconnect_timer;
    tcp::resolver m_resolver;
    boost::scoped_ptr<boost::asio::io_service::work> m_work;
};

} // end namespace simpletsdb

#endif

