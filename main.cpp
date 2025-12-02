#include "filehandler/filehandler.hpp"
#include "gst/gst.hpp"
#include "socket/communication.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
constexpr const int PORT = 12345;
// 진입점.
int main() try {

	// 저장되는 파일 위치
	std::string base_dir("video/");
	std::string fhd_dir("video/FHD/");
	std::string sd_dir("video/SD/");
	// 디렉토리 생성.
	create_video_dir(base_dir);
	create_video_dir(fhd_dir);
	create_video_dir(sd_dir);

	// gst 초기화
	gst_init(nullptr, nullptr);

	// 소켓 통신 스레드 분기
	std::thread sock_thread(socket_thread, PORT);
	sock_thread.detach();

	std::thread gst_handler_thread(handle_gst);
	gst_handler_thread.detach();
	const gchar *pipeline_sd =
		"udpsrc port=5000 "
		"caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,"
		"encoding-name=(string)H264,payload=(int)96\" ! "
		"rtpjitterbuffer latency=200 drop-on-latency=true ! "
		"rtph264depay ! "
		"decodebin ! "
		"videoconvert ! videoscale ! "
		"video/x-raw,format=(string)I420,width=(int)640,height=(int)480 ! "
		"x264enc tune=zerolatency bitrate=3000 speed-preset=superfast ! "
		"splitmuxsink name=recv location=\"video/SD/rec-%05d.mp4\" start-index=0 "
		"max-size-time=600000000000 muxer=mp4mux";

	const gchar *pipeline_fhd =
		"udpsrc port=5000 "
		"caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,"
		"encoding-name=(string)H264,payload=(int)96\" ! "
		"rtpjitterbuffer latency=200 drop-on-latency=true ! "
		"rtph264depay ! "
		"decodebin ! "
		"videoconvert ! videoscale ! "
		"video/x-raw,format=(string)I420,width=(int)1920,height=(int)1080 ! "
		"x264enc tune=zerolatency bitrate=9000 speed-preset=superfast ! "
		"splitmuxsink name=recv location=\"video/FHD/rec-%05d.mp4\" start-index=0 "
		"max-size-time=600000000000 muxer=mp4mux";

	while (true) {
		if (fh.gst_flag.load()) {
			do_gst(pipeline_fhd, fhd_dir);
		} else {
			do_gst(pipeline_sd, sd_dir);
		}
	}
	return 0;
} catch (const std::filesystem::filesystem_error &e) {
	std::cerr << e.what() << std::endl;
} catch (const pipeline_create_fail &e) {
	std::cerr << e.what() << std::endl;
} catch (const pipeline_playing_fail &e) {
	std::cerr << e.what() << std::endl;
} catch (std::exception &e) {
	std::cerr << "예외 발생: " << e.what() << std::endl;
} catch (...) {
	std::cerr << "Undefined Error" << std::endl;
}
