#include "song.hpp"
#include "yaml_io.hpp"
#include "playlist.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <music_directory>" << std::endl;
        return 1;
    }

    std::string directory = argv[1];
    std::vector<Song> songs = parse_all_songs(directory);
    write_songs_to_yaml(songs, "songs.yaml");
    generate_playlists_from_config(songs, "playlists.yaml", "playlists");
}