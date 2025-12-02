#include "filehandler.hpp"
#include <algorithm>
#include <filesystem>
#include <vector>

static std::string parse_index(std::string &filename) {
	const std::string prefix = "rec-";
	const std::string suffix = ".mp4";

	std::size_t pos1 = filename.find(prefix);
	if (pos1 == std::string::npos) {
		return "";
	}
	pos1 += prefix.size(); // 숫자가 시작하는 인덱스로 이동

	std::size_t pos2 = filename.rfind(suffix);
	if (pos2 == std::string::npos || pos2 <= pos1) {
		return "";
	}

	return filename.substr(pos1, pos2 - pos1);
}

int get_video_index(const std::string_view dirdir) {
	std::filesystem::path dir = dirdir;
	std::vector<std::string> filenames;
	for (const auto &entry : std::filesystem::directory_iterator(dir)) {
		if (std::filesystem::is_regular_file(entry.status())) {
			filenames.push_back(entry.path().filename().string());
		}
	}

	if (filenames.empty()) {
		return 0;
	}

	// 파일명 기준 내림차순 정렬
	std::sort(filenames.begin(), filenames.end(), std::greater<>());

	std::string fileindex = parse_index(filenames[0]);
	return std::stoi(fileindex) + 1;
}

void create_video_dir(const std::string_view path) {
	std::filesystem::path filepath = path;
	std::filesystem::path dirpath = filepath.parent_path();
	if (!std::filesystem::exists(dirpath)) {
		std::filesystem::create_directories(dirpath);
	}
}
