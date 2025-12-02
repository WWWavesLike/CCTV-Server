#include "communication.hpp"
#include "../gst/gst.hpp"
#include <iostream>
session::session(tcp::socket socket) : socket_(std::move(socket)) {}
void session::start() {
	do_read();
}
void session::do_read() {
	auto self(shared_from_this());
	socket_.async_read_some(
		boost::asio::buffer(data_, max_length - 1),
		[this, self](boost::system::error_code ec, std::size_t /* length*/) {
			if (!ec) {
				// std::cout << "수신 받은 데이터: " << data_ << std::endl;
				//  do_write(length);
				//  handler.appentd_json_object(data_);
				int dct_cnt{sql_insert(data_)};
				if (dct_cnt != -1) {
					fh.dct_cnt.store(dct_cnt);
				}
				memset(data_, 0, max_length);
				refresh_avg_dct();
				do_read();
			} else if (ec == boost::asio::error::eof) {
				// 클라이언트가 정상적으로 연결을 종료한 경우
				std::cout << "클라이언트가 연결을 종료했습니다." << std::endl;
			} else {
				std::cerr << "읽기 오류: " << ec.message() << std::endl;
			}
		});
}
server::server(boost::asio::io_context &io_context, short port)
	: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
}

// 비동기적으로 클라이언트 연결을 수락
void server::do_accept() {
	acceptor_.async_accept(
		[this](boost::system::error_code ec, tcp::socket socket) {
			if (!ec) {
				// 연결된 클라이언트 세션 생성 후 시작
				std::make_shared<session>(std::move(socket))->start();
			} else {
				std::cerr << "수락 오류: " << ec.message() << std::endl;
			}
			// 계속해서 새로운 연결을 수락
			do_accept();
		});
}
// 소켓 통신 진입점.
void socket_thread(const int port) {
	std::cout << "Server on... Port : " << port << std::endl;
	boost::asio::io_context io_context;

	server s(io_context, port);
	s.do_accept();
	io_context.run();
}
