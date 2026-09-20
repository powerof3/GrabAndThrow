#!/usr/bin/env python3
r"""
add_mcm_line.py - bulk-edit MCM translation files in a folder.

Two modes:
  * insert a line into every file          add_mcm_line.py '$Key\tValue'
  * rename a key in every file             add_mcm_line.py --rename $Old $New

Skyrim / Fallout 4 MCM translation files live in Interface/Translations/ and are
named MODNAME_LANGUAGE.txt. They are normally UTF-16 LE with a BOM, use CRLF
line endings, and each line looks like:

    $KeyName<TAB>Displayed text

This script preserves each file's original encoding, BOM and line endings.

Examples
--------
    # append a line to every translation file in the script's own folder
    python add_mcm_line.py '$MyNewKey\tMy new label'

    # insert so that it becomes line 5, in a folder given explicitly
    python add_mcm_line.py '$MyNewKey\tMy new label' ./Translations -n 5

    # insert next to an existing key, with a blank separator line
    python add_mcm_line.py '$MyNewKey\tMy new label' --after-key '$PM_Other' --blank

    # rename a key everywhere, keeping each file's translated text
    # (quote the keys - the shell eats a bare $Name)
    python add_mcm_line.py --rename '$OldKey' '$NewKey'

    # change the text shown for one key, in one language
    python add_mcm_line.py --set '$PM_FAVORITES' 'Favorite Forms' -l english

    # create the language files that don't exist yet, as copies of english
    python add_mcm_line.py --fill-missing --dry-run

    # keep .bak copies, and preview before writing
    python add_mcm_line.py '$Key\tValue' -l english,french --backup --dry-run
"""

from __future__ import annotations

import argparse
import codecs
import difflib
import shutil
import sys
from pathlib import Path

# Longest BOMs first: BOM_UTF32_LE starts with BOM_UTF16_LE.
BOM_TABLE = [
    (codecs.BOM_UTF32_LE, "utf-32-le"),
    (codecs.BOM_UTF32_BE, "utf-32-be"),
    (codecs.BOM_UTF8, "utf-8"),
    (codecs.BOM_UTF16_LE, "utf-16-le"),
    (codecs.BOM_UTF16_BE, "utf-16-be"),
]
FALLBACK_ENCODINGS = ("utf-8", "cp1252", "latin-1")

# Values Skyrim accepts for sLanguage, i.e. the files the game may look for.
DEFAULT_LANGUAGES = ("english", "czech", "french", "german", "italian",
                     "japanese", "polish", "russian", "spanish", "chinese")


# --------------------------------------------------------------------------- #
# encoding / text helpers
# --------------------------------------------------------------------------- #
def decode_bytes(raw: bytes) -> tuple[str, str, bytes]:
    """Return (text, encoding, bom) for a raw file."""
    for bom, enc in BOM_TABLE:
        if raw.startswith(bom):
            return raw[len(bom):].decode(enc), enc, bom
    for enc in FALLBACK_ENCODINGS:
        try:
            return raw.decode(enc), enc, b""
        except UnicodeDecodeError:
            continue
    raise ValueError("unable to decode file with any known encoding")


def detect_newline(text: str) -> str:
    if "\r\n" in text:
        return "\r\n"
    if "\r" in text:
        return "\r"
    if "\n" in text:
        return "\n"
    return "\r\n"  # Skyrim convention for a new / single-line file


def unescape(value: str) -> str:
    r"""Turn literal \t and \n typed on the command line into real characters."""
    return value.encode("utf-8").decode("unicode_escape")


def key_of(line: str) -> str | None:
    """The $Key part of an MCM line, if there is one."""
    stripped = line.strip()
    if not stripped.startswith("$"):
        return None
    return stripped.split("\t", 1)[0].strip()


def normalise_key(key: str) -> str:
    """MCM keys always start with '$' - add it if the user left it off."""
    key = key.strip()
    return key if key.startswith("$") else "$" + key


def same_key(a: str | None, b: str, ignore_case: bool) -> bool:
    if a is None:
        return False
    return a.lower() == b.lower() if ignore_case else a == b


# --------------------------------------------------------------------------- #
# transforms: take the file's lines, return (new_lines | None, message)
# --------------------------------------------------------------------------- #
def transform_insert(lines: list[str], args: argparse.Namespace):
    new_lines = args.new_lines
    meaningful = [l for l in new_lines if l.strip()]

    # Duplicate protection only applies to real content, never to blank lines.
    if meaningful and all(any(l.strip() == n.strip() for l in lines) for n in meaningful):
        return None, "identical line(s) already present"
    if not args.allow_duplicate_key:
        for n in meaningful:
            k = key_of(n)
            if k and any(same_key(key_of(l), k, args.ignore_case) for l in lines):
                return None, f"key {k} already present"

    # Where to put it.
    if args.after_key or args.before_key:
        target = normalise_key(args.after_key or args.before_key)
        pos = next((i for i, l in enumerate(lines)
                    if same_key(key_of(l), target, args.ignore_case)), None)
        if pos is None:
            return None, f"anchor {target} not found"
        index = pos + 1 if args.after_key else pos
    elif args.line_number is None:
        index = len(lines)
    elif args.line_number < 0:
        index = max(0, len(lines) + args.line_number + 1)
    else:
        index = max(0, min(args.line_number - 1, len(lines)))

    out = lines[:index] + new_lines + lines[index:]
    what = ("blank line" if not meaningful else
            f"{len(new_lines)} lines" if len(new_lines) > 1 else "line")
    return out, f"inserted {what} at line {index + 1}"


def missing_key_message(key: str, lines: list[str], ignore_case: bool) -> str:
    """Explain a key miss, suggesting near matches (ignoring case when matching)."""
    keys = [k for k in (key_of(l) for l in lines) if k]
    if not ignore_case and any(k.lower() == key.lower() for k in keys):
        return f"{key} not found (same key exists in another case - try -i)"
    lowered = {k.lower(): k for k in reversed(keys)}
    near = [lowered[m] for m in
            difflib.get_close_matches(key.lower(), list(lowered), n=3, cutoff=0.6)]
    hint = f" - did you mean {', '.join(near)}?" if near else ""
    return f"{key} not found{hint}"


def transform_set(lines: list[str], args: argparse.Namespace):
    key, value = args.set_key, args.set_value

    hits = [i for i, l in enumerate(lines) if same_key(key_of(l), key, args.ignore_case)]
    if not hits:
        return None, missing_key_message(key, lines, args.ignore_case)

    out = list(lines)
    changed = []
    for i in hits:
        head, sep, tail = out[i].partition("\t")
        if sep and tail == value:
            continue
        out[i] = head + "\t" + value
        changed.append(i)

    if not changed:
        return None, f"{key} already set to that text"
    where = ", ".join(str(i + 1) for i in changed)
    return out, f"{key} = {value!r} on line {where}"


def transform_rename(lines: list[str], args: argparse.Namespace):
    old, new = args.old_key, args.new_key

    hits = [i for i, l in enumerate(lines) if same_key(key_of(l), old, args.ignore_case)]
    if not hits:
        return None, missing_key_message(old, lines, args.ignore_case)
    if not args.allow_duplicate_key:
        clash = [i for i, l in enumerate(lines)
                 if i not in hits and same_key(key_of(l), new, args.ignore_case)]
        if clash:
            return None, f"{new} already exists on line {clash[0] + 1}"

    out = list(lines)
    for i in hits:
        head, sep, tail = out[i].partition("\t")
        out[i] = head.replace(head.strip(), new, 1) + sep + tail

    where = ", ".join(str(i + 1) for i in hits)
    return out, f"{old} -> {new} on line {where}"


# --------------------------------------------------------------------------- #
# per-file driver
# --------------------------------------------------------------------------- #
def process_file(path: Path, args: argparse.Namespace) -> str:
    raw = path.read_bytes()
    try:
        text, encoding, bom = decode_bytes(raw)
    except ValueError as exc:
        return f"ERROR  {path.name}: {exc}"

    newline = detect_newline(text)
    ends_with_newline = text.endswith(("\n", "\r")) or not text
    lines = text.splitlines()

    if args.rename:
        transform = transform_rename
    elif args.set:
        transform = transform_set
    else:
        transform = transform_insert
    new_lines, message = transform(lines, args)
    if new_lines is None:
        return f"skip   {path.name}: {message}"

    out_text = newline.join(new_lines)
    if ends_with_newline:
        out_text += newline

    out_encoding = args.encoding or encoding
    out_bom = bom
    if args.encoding:  # forced encoding: rebuild a matching BOM
        out_bom = next((b for b, e in BOM_TABLE if e == out_encoding), b"")
        if out_encoding.replace("-", "") == "utf8" and not args.bom:
            out_bom = b""
    payload = out_bom + out_text.encode(out_encoding)

    if args.dry_run:
        return f"would  {path.name}: {message} ({out_encoding})"

    if args.backup:
        shutil.copy2(path, path.with_suffix(path.suffix + ".bak"))
    path.write_bytes(payload)
    return f"ok     {path.name}: {message} ({out_encoding})"


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #
def match_case(sample: str, word: str) -> str:
    """Write `word` in the same case style as `sample` (ENGLISH/English/english)."""
    if sample.isupper():
        return word.upper()
    if sample[:1].isupper():
        return word.capitalize()
    return word.lower()


def fill_missing(files: list[Path], args: argparse.Namespace) -> int:
    src_lang = args.source_language.strip().lower()
    sources = [f for f in files if f.stem.rsplit("_", 1)[-1].lower() == src_lang]
    if not sources:
        print(f"error: no *_{src_lang}.txt file found in {args.folder}",
              file=sys.stderr)
        return 2

    wanted = ([l.strip().lower() for l in args.languages.split(",") if l.strip()]
              if args.languages else list(DEFAULT_LANGUAGES))
    wanted = [l for l in wanted if l != src_lang]
    if not wanted:
        print("error: no target languages left to create", file=sys.stderr)
        return 2

    # Every base/language pair already on disk, compared case-insensitively.
    present = set()
    for f in files:
        base, _, lang = f.stem.rpartition("_")
        if base:
            present.add((f.parent, base.lower(), lang.lower()))

    created = 0
    for src in sources:
        base, _, suffix = src.stem.rpartition("_")
        raw = src.read_bytes()
        if args.encoding:  # optional re-encode while copying
            text, enc, bom = decode_bytes(raw)
            out_bom = next((b for b, e in BOM_TABLE if e == args.encoding), b"")
            if args.encoding.replace("-", "") == "utf8" and not args.bom:
                out_bom = b""
            raw = out_bom + text.encode(args.encoding)

        for lang in wanted:
            dest = src.with_name(f"{base}_{match_case(suffix, lang)}{src.suffix}")
            if (src.parent, base.lower(), lang) in present:
                print(f"skip   {dest.name}: already exists")
                continue
            if args.dry_run:
                print(f"would  {dest.name}: copy of {src.name}")
            else:
                dest.write_bytes(raw)
                print(f"ok     {dest.name}: created from {src.name}")
            present.add((src.parent, base.lower(), lang))
            created += 1

    verb = "would be created" if args.dry_run else "created"
    print(f"\n{created} file(s) {verb}")
    return 0 if created else 1


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Add a line to, or rename a key in, every MCM translation "
                    "file in a folder.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="examples:\n" + __doc__.split("--------\n", 1)[-1],
    )
    p.add_argument("text", nargs="?", default=None,
                   help=r"the line to insert, e.g. '$MyKey\tMy Value' "
                        "(omit when using --rename)")
    p.add_argument("folder", type=Path, nargs="?", default=None,
                   help="folder containing the translation files "
                        "(default: the folder this script is in)")
    p.add_argument("--rename", nargs=2, metavar=("OLD", "NEW"), default=None,
                   help="rename key OLD to NEW in every file, keeping each "
                        "file's translated text ('$' optional)")
    p.add_argument("--fill-missing", action="store_true",
                   help="create any missing language files as copies of the "
                        "english one (use -l to choose which languages)")
    p.add_argument("--source-language", default="english", metavar="LANG",
                   help="language to copy from for --fill-missing "
                        "(default: %(default)s)")
    p.add_argument("--set", nargs=2, metavar=("KEY", "VALUE"), default=None,
                   help="replace the text shown for KEY with VALUE, keeping the "
                        "key itself (combine with -l to hit one language)")
    p.add_argument("-b", "--blank", action="store_true",
                   help="insert an empty line (no text needed)")
    p.add_argument("--after-key", default=None, metavar="KEY",
                   help="insert just after this key instead of a line number")
    p.add_argument("--before-key", default=None, metavar="KEY",
                   help="insert just before this key instead of a line number")
    p.add_argument("-n", "--line-number", type=int, default=None, metavar="N",
                   help="1-based line number the inserted line should occupy "
                        "(default: append at the end; negative counts from the end)")
    p.add_argument("-p", "--pattern", default="*.txt",
                   help="glob for files to touch (default: %(default)s)")
    p.add_argument("-l", "--languages", default=None,
                   help="comma-separated language suffixes to restrict to, "
                        "e.g. english,french,german")
    p.add_argument("-r", "--recursive", action="store_true",
                   help="also search subfolders")
    p.add_argument("-i", "--ignore-case", action="store_true",
                   help="match keys case-insensitively")
    p.add_argument("--raw", action="store_true",
                   help=r"do not convert \t and \n in the inserted line")
    p.add_argument("--encoding", default=None,
                   help="force output encoding (e.g. utf-16-le) instead of "
                        "keeping each file's own")
    p.add_argument("--bom", action="store_true",
                   help="with --encoding utf-8, also write a BOM")
    p.add_argument("--allow-duplicate-key", action="store_true",
                   help="proceed even if the key already exists in the file")
    p.add_argument("--backup", action="store_true",
                   help="write a .bak copy of each file before changing it")
    p.add_argument("--dry-run", action="store_true",
                   help="report what would change without writing anything")
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    # ---- resolve the two positionals, forgiving about order -------------- #
    if args.rename and args.folder is None and args.text is not None \
            and Path(args.text).is_dir():
        # "--rename A B ./Translations" lands the folder in `text`
        args.text, args.folder = None, Path(args.text)
    if args.folder is not None and args.text is not None \
            and Path(args.text).is_dir() and not args.folder.is_dir():
        args.text, args.folder = str(args.folder), Path(args.text)
    if not args.rename and args.folder is None and args.text is not None \
            and Path(args.text).is_dir():
        print("error: only a folder was given - supply the line to insert, "
              "or use --rename OLD NEW", file=sys.stderr)
        return 2

    chosen = [name for name, on in (("--rename", bool(args.rename)),
                                    ("--set", bool(args.set)),
                                    ("--fill-missing", args.fill_missing),
                                    ("a line to insert", args.text is not None
                                     or args.blank)) if on]
    if len(chosen) > 1:
        print(f"error: give only one of {', '.join(chosen)}", file=sys.stderr)
        return 2

    if args.folder is None:
        args.folder = Path(__file__).resolve().parent
        print(f"using folder: {args.folder}")

    if not args.folder.is_dir():
        print(f"error: '{args.folder}' is not a folder.", file=sys.stderr)
        print("       Pass the folder that holds the *_english.txt files, use '.' "
              "for the current\n       folder, or omit it to use the script's own "
              "folder.", file=sys.stderr)
        return 2

    # ---- mode ------------------------------------------------------------ #
    if args.after_key and args.before_key:
        print("error: use --after-key or --before-key, not both", file=sys.stderr)
        return 2

    if args.rename or args.set:
        supplied = args.rename or [args.set[0]]
        if not all(k.strip().strip("$") for k in supplied):
            print("error: a key came through empty. Your shell probably expanded "
                  "the '$'.\n       Quote it: --rename '$OldKey' '$NewKey'  or  "
                  "--set '$Key' 'New text'\n       (or leave the $ off entirely)",
                  file=sys.stderr)
            return 2
        if args.after_key or args.before_key or args.blank:
            print("error: --blank/--after-key/--before-key only apply when "
                  "inserting", file=sys.stderr)
            return 2

    if args.fill_missing:
        pass
    elif args.set:
        args.set_key = normalise_key(args.set[0])
        args.set_value = args.set[1] if args.raw else unescape(args.set[1])
    elif args.rename:
        args.old_key = normalise_key(args.rename[0])
        args.new_key = normalise_key(args.rename[1])
        if args.old_key == args.new_key:
            print("error: OLD and NEW keys are the same", file=sys.stderr)
            return 2
    elif args.text is not None or args.blank:
        raw_text = args.text or ""
        if not args.raw:
            raw_text = unescape(raw_text)
        args.new_lines = raw_text.replace("\r\n", "\n").split("\n")
        if args.blank and args.text:
            args.new_lines.append("")
    else:
        print("error: nothing to do - supply a line to insert, --rename OLD NEW, "
              "--set KEY VALUE, or --fill-missing",
              file=sys.stderr)
        return 2

    # ---- collect files --------------------------------------------------- #
    globber = args.folder.rglob if args.recursive else args.folder.glob
    files = sorted(f for f in globber(args.pattern) if f.is_file())

    if args.fill_missing:
        if not files:
            print("no .txt files found in that folder", file=sys.stderr)
            return 1
        return fill_missing(files, args)

    if args.languages:
        wanted = {s.strip().lower() for s in args.languages.split(",") if s.strip()}
        files = [f for f in files if f.stem.rsplit("_", 1)[-1].lower() in wanted]

    if not files:
        print("no matching translation files found", file=sys.stderr)
        return 1

    changed = 0
    for path in files:
        result = process_file(path, args)
        changed += not result.startswith(("skip", "ERROR"))
        print(result)

    verb = "would change" if args.dry_run else "changed"
    print(f"\n{changed} of {len(files)} file(s) {verb}")
    return 0 if changed else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BrokenPipeError:  # piping into head/more on some shells
        sys.stderr.close()
        raise SystemExit(0)
    except KeyboardInterrupt:
        raise SystemExit(130)
