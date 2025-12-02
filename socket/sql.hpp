#pragma once
#include <ctime>
#include <mariadb/conncpp.hpp>
#include <optional>

constexpr const char *sql_url = "jdbc:mariadb://localhost:3306/objdct";
constexpr const char *sql_user = "root";
constexpr const char *sql_pw = "비번";
constexpr const char *sql_db = "objdct";
void refresh_avg_dct();
static const char *weekday_names[] = {
	"일요일", "월요일", "화요일", "수요일",
	"목요일", "금요일", "토요일"};

std::optional<int> parse_obj_num(const char *data);
bool is_working_time();
int sql_insert(const char *msg);
int sql_select();
