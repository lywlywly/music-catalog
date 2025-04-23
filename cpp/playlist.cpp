#include "yaml_io.hpp"
#include "playlist.hpp"

std::vector<PlaylistConfig> load_playlist_config(const std::string &config_path)
{
    YAML::Node root = YAML::LoadFile(config_path);
    std::vector<PlaylistConfig> configs;

    for (const auto &node : root["playlists"])
    {
        PlaylistConfig config;
        config.name = node["name"].as<std::string>();
        if (node["sort_by"])
        {
            if (node["sort_by"].IsScalar())
            {
                config.sort_by.push_back(node["sort_by"].as<std::string>());
            }
            else if (node["sort_by"].IsSequence())
            {
                for (const auto &field : node["sort_by"])
                {
                    config.sort_by.push_back(field.as<std::string>());
                }
            }
        }

        if (node["conditions"])
        {
            for (const auto &it : node["conditions"])
            {
                std::string key = it.first.as<std::string>();
                FieldCondition cond;

                if (it.second.IsScalar())
                {
                    if (it.second.Tag().find("int") != std::string::npos)
                        cond.int_value = it.second.as<int>();
                    else
                        cond.str_value = it.second.as<std::string>();
                }
                else if (it.second["min"] || it.second["max"])
                {
                    if (it.second["min"])
                        cond.min = it.second["min"].as<int>();
                    if (it.second["max"])
                        cond.max = it.second["max"].as<int>();
                }
                else if (it.second["any"])
                {
                    for (const auto &val : it.second["any"])
                    {
                        cond.any.push_back(val.as<std::string>());
                    }
                }

                config.conditions[key] = cond;
            }
        }

        configs.push_back(config);
    }

    return configs;
}

static const std::unordered_map<std::string, MatchFunc> match_funcs = {
    {"title", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.title, c); }},
    {"album", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.album, c); }},
    {"discnumber", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.discnumber, c); }},
    {"tracknumber", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.tracknumber, c); }},
    {"path", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.path, c); }},
    {"date_added", [](const Song &s, const FieldCondition &c)
     { return match_string_field(s.date_added, c); }},
    {"rating", [](const Song &s, const FieldCondition &c)
     { return match_int_field(s.rating, c); }},
    {"artist", [](const Song &s, const FieldCondition &c)
     { return match_string_vector_field(s.artist, c); }},
    {"genre", [](const Song &s, const FieldCondition &c)
     { return match_string_vector_field(s.genre, c); }},
};

void generate_playlists_from_config(const std::vector<Song> &songs, const std::string &config_path, const std::string &out_dir)
{
    std::vector<PlaylistConfig> configs = load_playlist_config(config_path);

    for (const auto &cfg : configs)
    {
        std::vector<Song> selected;
        for (const auto &song : songs)
        {
            bool matched = true;
            for (const auto &[field, cond] : cfg.conditions)
            {
                if (!matches_condition(song, field, cond))
                {
                    matched = false;
                    break;
                }
            }
            if (matched)
                selected.push_back(song);
        }

        if (!cfg.sort_by.empty())
        {
            std::sort(selected.begin(), selected.end(), [&](const Song &a, const Song &b)
                      {
                          for (const auto &raw_key : cfg.sort_by)
                          {
                              bool descending = false;
                              std::string key = raw_key;
                              if (!key.empty() && key[0] == '-')
                              {
                                  descending = true;
                                  key = key.substr(1);
                              }

                              if (key == "rating")
                              {
                                  if (a.rating != b.rating)
                                      return descending ? a.rating < b.rating : a.rating > b.rating;
                              }
                              else if (key == "title")
                              {
                                  if (a.title != b.title)
                                      return descending ? a.title > b.title : a.title < b.title;
                              }
                              else if (key == "album")
                              {
                                  if (a.album != b.album)
                                      return descending ? a.album > b.album : a.album < b.album;
                              }
                              else if (key == "date_added")
                              {
                                  if (a.date_added != b.date_added)
                                      return descending ? a.date_added > b.date_added : a.date_added < b.date_added;
                              }
                          }
                          return a < b; // fallback to tie-break using <=>
                      });
        }

        std::ofstream out(out_dir + "/" + cfg.name + ".m3u8");
        for (const auto &song : selected)
        {
            out << "../music/" << song.path << "\n";
        }
        std::cout << "Wrote playlist: " << cfg.name << " (" << selected.size() << " tracks)\n";
    }
}

bool match_string_field(const std::string &value, const FieldCondition &cond)
{
    return !cond.str_value || value == *cond.str_value;
}

bool match_int_field(int value, const FieldCondition &cond)
{
    if (cond.int_value && value != *cond.int_value)
        return false;
    if (cond.min && value < *cond.min)
        return false;
    if (cond.max && value > *cond.max)
        return false;
    return true;
}

bool match_string_vector_field(const std::vector<std::string> &vec, const FieldCondition &cond)
{
    if (cond.str_value)
    {
        return std::find(vec.begin(), vec.end(), *cond.str_value) != vec.end();
    }
    if (!cond.any.empty())
    {
        for (const auto &val : cond.any)
        {
            if (std::find(vec.begin(), vec.end(), val) != vec.end())
                return true;
        }
        return false;
    }
    return true;
}

bool matches_condition(const Song &song, const std::string &field, const FieldCondition &cond)
{
    auto it = match_funcs.find(field);
    return it != match_funcs.end() ? it->second(song, cond) : false;
}