#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP
#include <string_view>

void create_video_dir(std::string_view path);

int get_video_index(const std::string_view dir);
#endif // !FILEHANDLER_HPP
