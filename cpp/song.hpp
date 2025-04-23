#pragma once

#include <algorithm>
#include <compare>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/tag.h>
#include <taglib/apefile.h>
#include <taglib/asffile.h>
#include <taglib/flacfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/opusfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavpackfile.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct Song
{
    std::string title;
    std::vector<std::string> artist;
    std::string album;
    std::vector<std::string> genre;
    int rating;
    std::string discnumber;
    std::string tracknumber;
    std::string path;
    std::string date_added;

    std::strong_ordering operator<=>(const Song &other) const;
    bool operator==(const Song &other) const;
};

int compare_str(const std::string &a, const std::string &b);
int compare_vec(const std::vector<std::string> &a, const std::vector<std::string> &b);
int get_num(const std::string &s);
int parse_number(const std::string &s);
std::string sanitize_filename(const std::string &input);
std::string join_artist(const std::vector<std::string> &artist);

Song parse_song_tags(const fs::path &path, const std::string &ext, TagLib::File *file);
std::vector<Song> parse_all_songs(const std::string &directory);

std::string get_lowercase_ext(const fs::path &path);
std::string get_first(const TagLib::PropertyMap &map, const std::string &key);
std::vector<std::string> get_list(const TagLib::PropertyMap &map, const std::string &key);

enum class FileType
{
    MPEG,
    FLAC,
    OGG_VORBIS,
    MP4,
    ASF,
    WAVPACK,
    APE,
    OPUS,
    UNKNOWN
};

static const std::map<std::string, FileType> extension_to_type = {
    {"mp3", FileType::MPEG},
    {"flac", FileType::FLAC},
    {"ogg", FileType::OGG_VORBIS},
    {"m4a", FileType::MP4},
    {"opus", FileType::OPUS}};

FileType detect_file_type(TagLib::File *file);
const char *to_string(FileType type);
void verify_file_extension(const fs::path &path, const std::string &ext, TagLib::FileRef &ref);
std::pair<int, std::string> parse_track_or_disc(const std::string &value);
std::string build_safe_filename(std::string_view artist_joined,
                                std::string_view album,
                                std::string_view disc_part,
                                std::string_view track_part,
                                std::string_view title,
                                std::string_view ext_out);
void try_rename_file(fs::path &old_path, const std::string &safe_filename);

static const std::vector<std::string> accepted_exts = {"flac", "m4a", "mp3", "ogg", "opus"};
