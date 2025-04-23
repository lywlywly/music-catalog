from create_songs_getter import PLAYLIST_DIRECTORY, Song, create_playlist


def song_sort_key(song: Song):
    return song["date_added"]


def main():
    songs = create_playlist()

    playlists = [
        ("Good", [song["path"] for song in songs if song["rating"] >= 8]),
        ("Pop", [song["path"] for song in songs if "Pop" in song["genre"]]),
        ("Classical", [song["path"] for song in songs if "Classical" in song["genre"]]),
        (
            "Recent",
            [song["path"] for song in sorted(songs, key=song_sort_key, reverse=True)],
        ),
    ]

    for playlist_name, song_paths in playlists:
        with open(f"{PLAYLIST_DIRECTORY}/{playlist_name}.m3u8", "w") as f:
            f.writelines(f"../music/{song_path}\n" for song_path in song_paths)


main() if __name__ == "__main__" else None
