// yaml_io.hpp
#pragma once

#include "song.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

bool is_yaml_sensitive(const std::string &s);
void emit_string(YAML::Emitter &out, const std::string &s);
void emit_string_vector(YAML::Emitter &out, const std::vector<std::string> &vec);
void emit_song(YAML::Emitter &out, const Song &s);
void write_songs_to_yaml(const std::vector<Song> &songs, const std::string &filename);
