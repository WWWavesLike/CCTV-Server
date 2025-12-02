#pragma once
#include "../socket/sql.hpp"
#include <atomic>
#include <gstreamer-1.0/gst/gst.h>
#include <stdexcept>
// void do_gst_thread(std::atomic<bool>& stop_flag, gst_base& g_obj);
constexpr const bool FHD = true;
constexpr const bool SD = false;

class pipeline_create_fail : public std::runtime_error {
public:
	explicit pipeline_create_fail();
};

class pipeline_playing_fail : public std::runtime_error {
public:
	explicit pipeline_playing_fail();
};

void do_gst(const gchar *pipeline_desc, const std::string_view dir);
// void do_gst_mult(const gchar *pipeline_desc, int i);
struct flag_handler {
	std::atomic<bool> stop_flag{false};
	// false : SD
	std::atomic<bool> gst_flag{SD};
	std::atomic<int> dct_cnt{0};
	std::atomic<int> avg_cnt{sql_select()};
};

extern flag_handler fh;
void handle_gst();
