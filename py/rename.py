import logging
import os

import taglib

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s - %(levelname)s - %(message)s",
)

file_handler = logging.FileHandler("rename.log", "w")
file_handler.setLevel(logging.DEBUG)
formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
file_handler.setFormatter(formatter)

logger = logging.getLogger()
logger.addHandler(file_handler)

DIRECTORY = "music"
VALID_EXTS = ["flac", "m4a", "mp3", "ogg", "opus", "wav"]


def escape_fn(fn: str):
    replacements = {
        "/": "$slash$",
        "\\": "$backslash$",
        "?": "$questionmark$",
        ":": "$colon$",
        "|": "$bar$",
        "<": "$leq$",
        ">": "$geq$",
        '"': "$doublequote$",
        "*": "$asterisk$",
    }

    for char, replacement in replacements.items():
        fn = fn.replace(char, replacement)

    return fn


def log_and_append_to_list(list_to_append: list[str], level: int, msg: str):
    logging.log(level=level, msg=msg)
    list_to_append.append(msg)


def main():
    logs: list[str] = []
    os.chdir(DIRECTORY)
    filenames = list(
        filter(lambda x: x.split(".")[-1] in VALID_EXTS, os.listdir())
    )

    for f in filenames:
        if "conflict" in f:
            log_and_append_to_list(
                logs, logging.ERROR, f"sync conflict file detected, aborting: {f}"
            )
            break
        extension = f.split(".")[-1]
        with taglib.File(f) as song:
            tags = song.tags
            title_list: list[str] = tags["TITLE"]
            assert len(title_list) == 1, title_list
            title: str = escape_fn(title_list[0])
            artist: list[str] = tags.get("ARTIST", ["$unknown$"])
            artist = [escape_fn(string) for string in artist]
            album_list: list[str] = tags.get("ALBUM", ["$unknown$"])
            assert len(album_list) == 1, album_list
            album = escape_fn(album_list[0])
            discnumber = tags.get("DISCNUMBER", ["$unknown$"])
            tracknumber = tags.get("TRACKNUMBER", ["$unknown$"])
            assert len(discnumber) == 1, discnumber
            assert len(tracknumber) == 1, tracknumber
            discnumber = (
                int(discnumber[0].split("/")[0])
                if discnumber[0] != "$unknown$"
                else "$unknown$"
            )
            tracknumber = (
                int(tracknumber[0].split("/")[0])
                if tracknumber[0] != "$unknown$"
                else "$unknown$"
            )
            fn = f"{";".join(artist)} - {album} - {discnumber} - {tracknumber} - {title}.{extension}"
            try:
                os.rename(f, fn)
            except OSError as e:
                log_and_append_to_list(logs, logging.ERROR, str(e))

    log_and_append_to_list(
        logs, logging.INFO, "Failed operations logged in rename.log."
    )
    os.chdir("..")

    return logs


main() if __name__ == "__main__" else None
