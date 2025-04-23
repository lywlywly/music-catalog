import json
import os
from datetime import datetime
from functools import cmp_to_key
from typing import TypedDict

import icu
import taglib


class Song(TypedDict):
    title: str
    artist: list[str]
    album: str
    genre: list[str]
    rating: int
    discnumber: str
    tracknumber: str
    path: str
    date_added: datetime


class AdditionalInfo(TypedDict):
    lang: str
    genre: list[str]
    rating: int
    date_added: str


DIRECTORY = "music"
PLAYLIST_DIRECTORY = "playlists"
VALID_EXTS = ["flac", "m4a", "mp3", "ogg", "opus", "wav"]

loc = icu.Locale.forLanguageTag("zh-u-kr-latn-hani-hrkt")
# ordering based on simplified chinese pinyin, default ordering for other characters. Keep latin letters first, then Han (Hanzi, Kanji, Hanja), then Hiragana and Katakana
collator_alt = icu.Collator.createInstance(loc)


def get_num(s: str) -> int:
    """
    Return an integer from the string if possible, otherwise return 9999
    to sort not applicable values later.
    """
    try:
        return int(s)
    except ValueError:
        return 9999


def compare_field(x: int | str | list[str], y: int | str | list[str]) -> int:
    match x, y:
        case int(), int():
            return (x > y) - (x < y)
        case str(), str():
            return collator_alt.compare(x, y)
        case list(), list():
            for _x, _y in zip(x, y):
                comp_result = collator_alt.compare(_x, _y)
                if comp_result != 0:
                    return comp_result
            return 0
        case _:
            raise Exception(
                "x and y should be of the same type (int | str | list[str])"
            )


def get_referent(song: Song):
    return (
        song["artist"],
        song["album"],
        get_num(song["discnumber"]),
        get_num(song["tracknumber"]),
        song["title"],
    )


def compare_song(song: Song, other: Song) -> int:
    referent1 = get_referent(song)
    referent2 = get_referent(other)

    for x, y in zip(referent1, referent2):
        comp_result = compare_field(x, y)
        if comp_result > 0:
            return 1
        elif comp_result < 0:
            return -1
    return 0


def create_songs_getter():
    songs: list[Song] = list()

    def _create_songs():
        _songs: list[Song] = list()
        filenames = list(
            filter(lambda x: x.split(".")[-1] in VALID_EXTS, os.listdir(DIRECTORY))
        )

        for f in filenames:
            ext = f.split(".")[-1]
            if ext == "m4a":
                print(
                    f'Reading metadata from m4a file {f}, additional info from "comment" will be used.'
                )
            with taglib.File(f"{DIRECTORY}/{f}") as song:
                tags = song.tags
                title_list: list[str] = tags["TITLE"]
                assert len(title_list) == 1, title_list
                title: str = title_list[0]
                artist: list[str] = tags.get("ARTIST", ["$unknown$"])
                album_list: list[str] = tags.get("ALBUM", ["$unknown$"])
                assert len(album_list) == 1, album_list
                album = album_list[0]
                discnumber = tags.get("DISCNUMBER", ["$unknown$"])
                tracknumber = tags.get("TRACKNUMBER", ["$unknown$"])
                assert len(discnumber) == 1, discnumber
                assert len(tracknumber) == 1, tracknumber
                discnumber = discnumber[0].split("/")[0]
                tracknumber = tracknumber[0].split("/")[0]

                if ext == "m4a":
                    additional_info: AdditionalInfo = json.loads(tags["COMMENT"][0])
                    rating = int(additional_info["rating"])
                    genre = additional_info["genre"]
                    dt = datetime.strptime(
                        additional_info["date_added"], "%Y-%m-%d %H:%M:%S%z"
                    )
                    print(f'Additional info from "comment" successful.')
                else:
                    genre: list[str] = tags.get("GENRE", ["$unknown$"])
                    rating_list: list[str] = tags.get("RATING", ["$unknown$"])
                    assert len(rating_list) == 1, rating_list
                    try:
                        rating = int(rating_list[0])
                    except ValueError:
                        rating = -1
                        print(f"Rating not a number in file {f}")
                    date_time = tags.get("DATE_ADDED", ["$unknown$"])
                    dt = datetime.strptime(date_time[0], "%Y-%m-%d %H:%M:%S%z")

                _songs.append(
                    {
                        "title": title,
                        "artist": artist,
                        "album": album,
                        "genre": genre,
                        "rating": rating,
                        "discnumber": discnumber,
                        "tracknumber": tracknumber,
                        "path": f,
                        "date_added": dt,
                    }
                )

        _songs.sort(key=cmp_to_key(compare_song))
        return _songs

    def get_cached_songs():
        nonlocal songs
        if not songs:
            songs = _create_songs()
        return songs

    return get_cached_songs


create_playlist = create_songs_getter()
