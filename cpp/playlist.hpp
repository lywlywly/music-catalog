#pragma once

#include "song.hpp"
#include <optional>
#include <unordered_map>

struct FieldCondition
{
    std::optional<int> int_value;
    std::optional<int> min;
    std::optional<int> max;
    std::optional<std::string> str_value;
    std::vector<std::string> any;
};

struct PlaylistConfig
{
    std::string name;
    std::map<std::string, FieldCondition> conditions;
    std::vector<std::string> sort_by; // may include "-field" for descending
};

std::vector<PlaylistConfig> load_playlist_config(const std::string &config_path);
void generate_playlists_from_config(const std::vector<Song> &songs, const std::string &config_path, const std::string &out_dir);

bool match_string_field(const std::string &value, const FieldCondition &cond);
bool match_int_field(int value, const FieldCondition &cond);
bool match_string_vector_field(const std::vector<std::string> &vec, const FieldCondition &cond);
bool matches_condition(const Song &song, const std::string &field, const FieldCondition &cond);

using MatchFunc = std::function<bool(const Song &, const FieldCondition &)>;
