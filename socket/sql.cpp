#include "sql.hpp"
#include "../gst/gst.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>

std::optional<int> parse_obj_num(const char *data) {
	std::string msg(data);
	const std::string key1 = "Detected ";
	const std::string key2 = " objects";
	std::optional<int> retval;
	size_t pos1 = msg.find(key1);

	if (pos1 == std::string::npos) {
		return retval;
	}

	pos1 += key1.length(); // 숫자가 시작하는 위치

	size_t pos2 = msg.find(key2, pos1);
	if (pos2 == std::string::npos) {
		return retval;
	}

	std::string num_str = msg.substr(pos1, pos2 - pos1);

	retval = std::stoi(num_str);
	return retval;
}
bool is_working_time() {
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	// tm 구조체에 로컬타임을 채웁니다 (POSIX 환경)
	std::tm local_tm;
	localtime_r(&t, &local_tm);

	int hour = local_tm.tm_hour; // 0~23 범위
	return (hour >= 6 && hour < 18);
}
std::tm get_week() {
	std::time_t now = std::time(nullptr);	  // 현재 시각(UTC)을 time_t로 취득
	std::tm local_tm = *std::localtime(&now); // 로컬 타임으로 변환
	return local_tm;
}

int sql_insert(const char *data) {
	auto dct_opt = parse_obj_num(data);
	if (!dct_opt.has_value()) {
		std::cerr << "Parsing failed" << std::endl;
		return -1;
	}

	sql::Driver *driver = sql::mariadb::get_driver_instance();
	sql::SQLString url(sql_url);
	sql::Properties props({{"user", sql_user}, {"password", sql_pw}});
	std::unique_ptr<sql::Connection> conn(driver->connect(url, props));
	conn->setSchema(sql_db);
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(
			conn->prepareStatement(
				"INSERT INTO log (week, is_wt, dct_cnt, mes) VALUES (?, ?, ?, ?)"));
		pstmt->setString(1, weekday_names[get_week().tm_wday]);
		pstmt->setBoolean(2, is_working_time());
		pstmt->setInt64(3, dct_opt.value());
		pstmt->setString(4, data);
		pstmt->executeUpdate();
	}
	return dct_opt.value();
}
int sql_select() {
	sql::Driver *driver = sql::mariadb::get_driver_instance();
	sql::SQLString url(sql_url);
	sql::Properties props({{"user", sql_user}, {"password", sql_pw}});
	std::unique_ptr<sql::Connection> conn(driver->connect(url, props));
	conn->setSchema(sql_db);
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(
			conn->prepareStatement("SELECT round(avg(dct_cnt)) as avg FROM log group by week, is_wt having is_wt = ? and week = ?"));
		pstmt->setBoolean(1, is_working_time());
		pstmt->setString(2, weekday_names[get_week().tm_wday]);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			return res->getInt("avg");
		}
	}
	return 0;
}

void refresh_avg_dct() {
	static int timer = 0;
	if (timer++ >= 600) {
		fh.avg_cnt.store(sql_select());
		timer = 0;
	}
}

void change_gst_flag() {}
