#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP
#include <boost/asio.hpp>
#include <memory>
using boost::asio::ip::tcp;

void socket_thread(const int port);
class session : public std::enable_shared_from_this<session> {
public:
	session(tcp::socket socket);
	void start();

private:
	tcp::socket socket_;
	enum {
		max_length = 501
	};
	char data_[max_length];
	void do_read();
	//	void do_write(std::size_t length);
};

class server {
public:
	server(boost::asio::io_context &io_context, short port);
	void do_accept();

private:
	tcp::acceptor acceptor_;
};

#endif
