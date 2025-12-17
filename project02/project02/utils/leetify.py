#!/usr/bin/env python3
"""
to_leet.py

Usage:
    python to_leet.py input.txt
    python to_leet.py input.txt -o output_leet.txt

The script reads input.txt (UTF-8), converts characters to "l33tsp34k"
using a mapping, and writes the translated text to the output file.
If -o is omitted, the output filename is created by inserting "_leet"
before the input file extension.
"""

import argparse
from pathlib import Path
import sys

# Basic, readable leet mapping (deterministic)
LEET_MAP = {
    "a": "4",
    "b": "8",
    "e": "3",
    "f": "ph",
    "i": "1",
    "l": "1",
    "o": "0",
    "s": "5",
    "t": "7",
}

def translate_char(ch: str) -> str:
    """
    Translate a single character to leet. Preserve non-letter characters.
    If the char is a letter, we look up lowercase mapping and return that mapping.
    We do not try to "upper-case" leet symbols because many contain punctuation.
    """
    if not ch.isalpha():
        return ch
    mapped = LEET_MAP.get(ch.lower(), ch)
    # Optional: if original was uppercase and mapping is alphabetic-only,
    # we could uppercase the mapping. But most mappings include symbols; keep as-is.
    return mapped

def translate_text(text: str) -> str:
    """Translate full text into leet using translate_char for each character."""
    return "".join(translate_char(c) for c in text)

def default_output_path(inp: Path) -> Path:
    """Create default output filename: name_leet.ext or name_leet if no ext."""
    if inp.suffix:
        return inp.with_name(inp.stem + "_leet" + inp.suffix)
    else:
        return inp.with_name(inp.name + "_leet")

def main(argv=None):
    parser = argparse.ArgumentParser(description="Translate a text file to l33tsp34k.")
    parser.add_argument("input", help="Path to input text file", type=Path)
    parser.add_argument("-o", "--output", help="Path to output file (optional)", type=Path)
    args = parser.parse_args(argv)

    inp = args.input
    if not inp.exists():
        print(f"Error: input file does not exist: {inp}", file=sys.stderr)
        sys.exit(2)

    out = args.output or default_output_path(inp)

    try:
        text = inp.read_text(encoding="utf-8")
    except Exception as e:
        print(f"Error reading {inp}: {e}", file=sys.stderr)
        sys.exit(3)

    translated = translate_text(text)

    try:
        out.write_text(translated, encoding="utf-8")
    except Exception as e:
        print(f"Error writing {out}: {e}", file=sys.stderr)
        sys.exit(4)

    print(f"Wrote translated text to: {out}")

if __name__ == "__main__":
    main()
