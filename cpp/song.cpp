// song.cpp
#include "song.hpp"

#include <unicode/coll.h>
#include <unicode/locid.h>
#include <nlohmann/json.hpp>

UErrorCode status = U_ZERO_ERROR;
icu::Locale loc = icu::Locale::forLanguageTag("zh-u-kr-latn-hani-hrkt", status);
// # ordering based on simplified chinese pinyin, default ordering for other characters. Keep latin letters first, then Han (Hanzi, Kanji, Hanja), then Hiragana and Katakana
std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(loc, status));

std::strong_ordering Song::operator<=>(const Song &other) const
{
    if (int cmp = compare_vec(artist, other.artist); cmp != 0)
        return cmp < 0 ? std::strong_ordering::less : std::strong_ordering::greater;

    if (int cmp = compare_str(album, other.album); cmp != 0)
        return cmp < 0 ? std::strong_ordering::less : std::strong_ordering::greater;

    int d1 = get_num(discnumber), d2 = get_num(other.discnumber);
    if (d1 != d2)
        return d1 < d2 ? std::strong_ordering::less : std::strong_ordering::greater;

    int t1 = get_num(tracknumber), t2 = get_num(other.tracknumber);
    if (t1 != t2)
        return t1 < t2 ? std::strong_ordering::less : std::strong_ordering::greater;

    if (int cmp = compare_str(title, other.title); cmp != 0)
        return cmp < 0 ? std::strong_ordering::less : std::strong_ordering::greater;

    return std::strong_ordering::equal;
}

bool Song::operator==(const Song &other) const
{
    return compare_vec(artist, other.artist) == 0 &&
           compare_str(album, other.album) == 0 &&
           get_num(discnumber) == get_num(other.discnumber) &&
           get_num(tracknumber) == get_num(other.tracknumber) &&
           compare_str(title, other.title) == 0;
}

int compare_str(const std::string &a, const std::string &b)
{
    icu::UnicodeString ua = icu::UnicodeString::fromUTF8(a);
    icu::UnicodeString ub = icu::UnicodeString::fromUTF8(b);
    return collator->compare(ua, ub);
}

int compare_vec(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
    {
        int cmp = compare_str(a[i], b[i]);
        if (cmp != 0)
            return cmp;
    }
    // use size to break tie
    if (a.size() < b.size())
        return -1;
    if (a.size() > b.size())
        return 1;
    return 0;
}

int get_num(const std::string &s)
{
    try
    {
        return std::stoi(s);
    }
    catch (...)
    {
        return 9999;
    }
}

int parse_number(const std::string &s)
{
    try
    {
        return std::stoi(s);
    }
    catch (...)
    {
        throw std::runtime_error(s);
    }
}

std::string sanitize_filename(const std::string &input)
{
    static const std::unordered_map<char, std::string> replacements = {
        {'/', "$slash$"},
        {'\\', "$backslash$"},
        {'?', "$questionmark$"},
        {':', "$colon$"},
        {'|', "$bar$"},
        {'<', "$leq$"},
        {'>', "$geq$"},
        {'"', "$doublequote$"},
        {'*', "$asterisk$"}};

    std::string result;
    for (char c : input)
    {
        auto it = replacements.find(c);
        if (it != replacements.end())
        {
            result += it->second;
        }
        else
        {
            result += c;
        }
    }
    return result;
}

std::string join_artist(const std::vector<std::string> &artist)
{
    std::string result;
    for (size_t i = 0; i < artist.size(); ++i)
    {
        result += artist[i];
        if (i + 1 < artist.size())
            result += ";";
    }
    return result;
}

Song parse_song_tags(const fs::path &path, const std::string &ext, TagLib::File *file)
{
    TagLib::PropertyMap props = file->properties();

    std::string title = get_first(props, "TITLE");
    std::string album = get_first(props, "ALBUM");
    std::string discnumber = get_first(props, "DISCNUMBER");
    std::string tracknumber = get_first(props, "TRACKNUMBER");
    std::vector<std::string> artist = get_list(props, "ARTIST");
    if (artist.empty())
        artist = {"$unknown$"};
    std::vector<std::string> genre = get_list(props, "GENRE");
    if (genre.empty())
        genre = {"$unknown$"};

    int rating = -1;
    std::string date_added;

    if (ext == "m4a")
    {
        std::string comment = get_first(props, "COMMENT");
        try
        {
            auto j = json::parse(comment);
            genre = j["genre"].get<std::vector<std::string>>();
            rating = j["rating"];
            date_added = j["date_added"];
        }
        catch (...)
        {
            std::cerr << "Invalid JSON COMMENT in " << path << "\n";
        }
    }
    else
    {
        std::string rating_str = get_first(props, "RATING");
        try
        {
            rating = std::stoi(rating_str);
        }
        catch (...)
        {
            std::cerr << "Invalid RATING in " << path << "\n";
            rating = -1;
        }
        date_added = get_first(props, "DATE_ADDED");
    }

    return Song{
        .title = title,
        .artist = artist,
        .album = album,
        .genre = genre,
        .rating = rating,
        .discnumber = discnumber,
        .tracknumber = tracknumber,
        .path = path.filename().string(),
        .date_added = date_added};
}

std::vector<Song> parse_all_songs(const std::string &directory)
{
    std::vector<Song> songs;

    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (!entry.is_regular_file())
            continue;

        fs::path path = entry.path();
        std::string ext = get_lowercase_ext(path);

        if (std::find(accepted_exts.begin(), accepted_exts.end(), ext) == accepted_exts.end())
            continue;

        TagLib::FileRef ref(path.c_str());
        verify_file_extension(path, ext, ref);

        Song song = parse_song_tags(path, ext, ref.file());

        std::string artist_joined = join_artist(song.artist);
        auto [disc_int, disc_part] = parse_track_or_disc(song.discnumber);
        auto [track_int, track_part] = parse_track_or_disc(song.tracknumber);

        std::string ext_out = path.extension().string();
        std::string_view ext_out_view = ext_out;
        if (!ext_out_view.empty() && ext_out_view[0] == '.')
            ext_out_view.remove_prefix(1);

        std::string safe_filename = build_safe_filename(artist_joined, song.album, disc_part, track_part, song.title, ext_out_view);
        try_rename_file(path, safe_filename);

        song.path = path.filename().string();
        songs.push_back(song);
    }

    std::sort(songs.begin(), songs.end());
    return songs;
}

std::string get_lowercase_ext(const fs::path &path)
{
    auto ext = path.extension().string();
    if (!ext.empty() && ext[0] == '.')
        ext.erase(0, 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

std::string get_first(const TagLib::PropertyMap &map, const std::string &key)
{
    auto it = map.find(key);
    if (it != map.end())
    {
        const auto &list = it->second;
        if (list.size() == 1)
        {
            return list.front().to8Bit(true);
        }
        else
        {
            std::cerr << "Warning: Property \"" << key << "\" has "
                      << list.size() << " values; expected exactly 1.\n";
        }
    }
    return "$unknown$";
}

std::vector<std::string> get_list(const TagLib::PropertyMap &map, const std::string &key)
{
    std::vector<std::string> result;
    auto it = map.find(key);
    if (it != map.end())
    {
        for (const auto &val : it->second)
        {
            result.push_back(val.to8Bit(true));
        }
    }
    return result;
}

FileType detect_file_type(TagLib::File *file)
{
    if (!file)
        return FileType::UNKNOWN;
    if (dynamic_cast<TagLib::MPEG::File *>(file))
        return FileType::MPEG;
    if (dynamic_cast<TagLib::FLAC::File *>(file))
        return FileType::FLAC;
    if (dynamic_cast<TagLib::Vorbis::File *>(file))
        return FileType::OGG_VORBIS;
    if (dynamic_cast<TagLib::MP4::File *>(file))
        return FileType::MP4;
    if (dynamic_cast<TagLib::ASF::File *>(file))
        return FileType::ASF;
    if (dynamic_cast<TagLib::WavPack::File *>(file))
        return FileType::WAVPACK;
    if (dynamic_cast<TagLib::APE::File *>(file))
        return FileType::APE;
    if (dynamic_cast<TagLib::Ogg::Opus::File *>(file))
        return FileType::OPUS;
    return FileType::UNKNOWN;
}

const char *to_string(FileType type)
{
    switch (type)
    {
    case FileType::MPEG:
        return "MPEG (MP3)";
    case FileType::FLAC:
        return "FLAC";
    case FileType::OGG_VORBIS:
        return "Ogg Vorbis";
    case FileType::MP4:
        return "MP4/M4A";
    case FileType::ASF:
        return "ASF (WMA)";
    case FileType::WAVPACK:
        return "WavPack";
    case FileType::APE:
        return "APE";
    case FileType::OPUS:
        return "Ogg Opus";
    default:
        return "Unknown";
    }
}

void verify_file_extension(const fs::path &path, const std::string &ext, TagLib::FileRef &ref)
{
    if (ref.isNull() || !ref.file())
        return;
    auto it = extension_to_type.find(ext);
    if (it == extension_to_type.end())
        return;

    FileType detected = detect_file_type(ref.file());
    FileType expected = it->second;

    if (detected != expected)
    {
        std::ostringstream oss;
        oss << "File extension mismatch:\n"
            << "  File      : " << path << "\n"
            << "  Extension : " << ext << " (" << to_string(expected) << ")\n"
            << "  Detected  : " << to_string(detected) << "\n";
        throw std::runtime_error(oss.str());
    }
}

std::pair<int, std::string> parse_track_or_disc(const std::string &value)
{
    if (value != "$unknown$")
    {
        int num = parse_number(value.substr(0, value.find('/')));
        if (num >= 0)
            return {num, std::to_string(num)};
    }
    return {-1, "$unknown$"};
}

std::string build_safe_filename(std::string_view artist_joined,
                                std::string_view album,
                                std::string_view disc_part,
                                std::string_view track_part,
                                std::string_view title,
                                std::string_view ext_out)
{
    std::string raw_filename = std::string(artist_joined) + " - " + std::string(album) + " - " +
                               std::string(disc_part) + " - " + std::string(track_part) + " - " +
                               std::string(title) + "." + std::string(ext_out);
    return sanitize_filename(raw_filename);
}

void try_rename_file(fs::path &old_path, const std::string &safe_filename)
{
    fs::path new_path = old_path.parent_path() / safe_filename;
    if (new_path != old_path)
    {
        try
        {
            fs::rename(old_path, new_path);
            old_path = new_path;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to rename: " << old_path << " → " << new_path
                      << "\nReason: " << e.what() << "\n";
        }
    }
}