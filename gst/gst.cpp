#include "gst.hpp"
#include "../filehandler/filehandler.hpp"
#include <chrono>
#include <iostream>
#include <thread>
flag_handler fh;

void handle_gst() {
	while (true) {
		if (fh.dct_cnt.load() > fh.avg_cnt.load()) {
			if (fh.gst_flag.load() != FHD) {
				//				std::cout << "convert fhd" << std::endl;
				fh.gst_flag.store(true, std::memory_order_relaxed);
				fh.stop_flag.store(true, std::memory_order_relaxed);
			}
			std::this_thread::sleep_for(std::chrono::minutes(5));
		} else {
			if (fh.gst_flag.load() != SD) {
				std::cout << "convert sd" << std::endl;
				fh.gst_flag.store(false, std::memory_order_relaxed);
				fh.stop_flag.store(true, std::memory_order_relaxed);
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
void do_gst(const gchar *pipeline_desc, const std::string_view dir) {
	GError *error = nullptr;
	GstElement *pipeline = gst_parse_launch(pipeline_desc, &error);
	if (!pipeline) {
		g_error_free(error);
		throw pipeline_create_fail();
	}

	GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "recv");
	int index = get_video_index(dir);
	if (dir == "video/FHD/") {
		g_object_set(sink,
					 "location", "video/FHD/rec-%05d.mp4",
					 "start-index", index,
					 nullptr);
	} else {
		g_object_set(sink,
					 "location", "video/SD/rec-%05d.mp4",
					 "start-index", index,
					 nullptr);
	}
	gst_object_unref(sink);

	if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
		gst_object_unref(pipeline);
		throw pipeline_playing_fail();
	}

	GstBus *bus = gst_element_get_bus(pipeline);
	bool terminate = false;
	bool eos_sent = false;

	while (!terminate) {
		// stop_flag가 올라오면 한 번만 EOS 이벤트 전송
		if (fh.stop_flag.load(std::memory_order_relaxed) && !eos_sent) {
			gst_element_send_event(pipeline, gst_event_new_eos());
			eos_sent = true;
		}

		// EOS 전송 전에는 짧게, 전송 후에는 무한 대기
		GstMessage *msg = gst_bus_timed_pop_filtered(
			bus,
			eos_sent ? GST_CLOCK_TIME_NONE : 100 * GST_MSECOND,
			static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

		if (!msg)
			continue;

		switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR: {
			GError *err = nullptr;
			gchar *dbg = nullptr;
			gst_message_parse_error(msg, &err, &dbg);

			std::cerr << "Error from " << GST_OBJECT_NAME(msg->src)
					  << ": " << (err ? err->message : "unknown") << std::endl;
			std::cerr << "Debug info: " << (dbg ? dbg : "none") << std::endl;

			g_clear_error(&err);
			g_free(dbg);
			terminate = true;
			break;
		}
		case GST_MESSAGE_EOS: {
			std::cout << "End-Of-Stream reached." << std::endl;
			terminate = true;
			break;
		}
		default:
			break;
		}

		gst_message_unref(msg);
	}

	// 1) NULL 상태로 전이
	gst_element_set_state(pipeline, GST_STATE_NULL);
	// 2) 상태 전이 완료될 때까지 블록 대기
	gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

	gst_object_unref(bus);
	gst_object_unref(pipeline);

	//	std::cout << "STOP 플래그 비활성화" << std::endl;
	fh.stop_flag.store(false, std::memory_order_relaxed);
}

/*
void do_gst_mult(const gchar *pipeline_desc, int i) {
	const std::string base_dir("video/");
	const std::string dir(base_dir + std::to_string(i) + "/");
	const std::string dir_rec(dir + "rec-%05d.mp4");
	GError *error = nullptr;
	GstElement *pipeline = gst_parse_launch(pipeline_desc, &error);
	if (!pipeline) {
		g_error_free(error);
		throw pipeline_create_fail();
	}

	GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "recv");
	int index = get_video_index(dir);
	g_object_set(sink,
				 "location", dir_rec.c_str(),
				 "start-index", index,
				 nullptr);
	gst_object_unref(sink);

	if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
		gst_object_unref(pipeline);
		throw pipeline_playing_fail();
	}

	GstBus *bus = gst_element_get_bus(pipeline);
	bool terminate = false;
	bool eos_sent = false;

	//	std::cout << "GST 루프 시작" << std::endl;
	while (!terminate) {
		// stop_flag가 올라오면 한 번만 EOS 이벤트 전송
		if (fh.stop_flag.load(std::memory_order_relaxed) && !eos_sent) {
			gst_element_send_event(pipeline, gst_event_new_eos());
			eos_sent = true;
		}
// EOS 전송 전에는 짧게, 전송 후에는 무한 대기
GstMessage *msg = gst_bus_timed_pop_filtered(
	bus,
	eos_sent ? GST_CLOCK_TIME_NONE : 100 * GST_MSECOND,
	static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

if (!msg)
	continue;

switch (GST_MESSAGE_TYPE(msg)) {
case GST_MESSAGE_ERROR: {
	GError *err = nullptr;
	gchar *dbg = nullptr;
	gst_message_parse_error(msg, &err, &dbg);

	std::cerr << "Error from " << GST_OBJECT_NAME(msg->src)
			  << ": " << (err ? err->message : "unknown") << std::endl;
	std::cerr << "Debug info: " << (dbg ? dbg : "none") << std::endl;

	g_clear_error(&err);
	g_free(dbg);
	terminate = true;
	break;
}
case GST_MESSAGE_EOS: {
	std::cout << "End-Of-Stream reached." << std::endl;
	terminate = true;
	break;
}
default:
	break;
}

gst_message_unref(msg);
}

//	std::cout << "GST 루프 종료" << std::endl;

// 1) NULL 상태로 전이
gst_element_set_state(pipeline, GST_STATE_NULL);
// 2) 상태 전이 완료될 때까지 블록 대기
gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

gst_object_unref(bus);
gst_object_unref(pipeline);

//	std::cout << "STOP 플래그 비활성화" << std::endl;
//	fh.stop_flag.store(false, std::memory_order_relaxed);
}
*/

pipeline_create_fail::pipeline_create_fail() : std::runtime_error("파이프라인 생성 실패") {}
pipeline_playing_fail::pipeline_playing_fail() : std::runtime_error("파이프라인을 PLAYING 상태로 전환하지 못했습니다.") {}
