#include <iostream>
#include <mariadb/conncpp.hpp>
#include <memory>

int main() {
	// 1) 드라이버 인스턴스 획득
	sql::Driver *driver = sql::mariadb::get_driver_instance();
	// 2) 연결 정보 설정
	sql::SQLString url("jdbc:mariadb://localhost:3306/objdct");
	sql::Properties props({{"user", "root"}, {"password", "@Oldingdog12"}});
	// 3) 연결 생성
	std::unique_ptr<sql::Connection> conn(driver->connect(url, props));
	conn->setSchema("objdct");
	{
		std::unique_ptr<sql::Statement> stmt(conn->createStatement());
		int affected = stmt->executeUpdate(
			"INSERT INTO log (is_wt, dct_cnt,mes) "
			"VALUES (true, 666, '테스트')");
		std::cout << "[Statement] 삽입된 행 개수: " << affected << "\n";
	}
}
