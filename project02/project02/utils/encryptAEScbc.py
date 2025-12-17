#!/usr/bin/env python3
"""
AES-CBC (128-bit) file encryptor with PKCS7 padding.

Usage:
  python aes_cbc_encrypt.py -i input.bin -o encrypted.bin -k 00112233445566778899aabbccddeeff
  python aes_cbc_encrypt.py --generate-key    # prints a secure random 128-bit key (hex) and exits
"""

import argparse
import os
import sys
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

def parse_args():
    p = argparse.ArgumentParser(description="AES-CBC (128-bit) file encryptor with PKCS7 padding")
    p.add_argument("-i", "--infile", help="Input file to encrypt", required=False)
    p.add_argument("-o", "--outfile", help="Output file to write (IV + ciphertext)", required=False)
    p.add_argument("-k", "--key", help="128-bit key as 32-hex characters (e.g. 001122...)", required=False)
    p.add_argument("--generate-key", action="store_true", help="Generate a random 128-bit key (hex) and exit")
    return p.parse_args()

def gen_key_hex():
    return os.urandom(16).hex()

def validate_and_decode_key(hex_key: str) -> bytes:
    if hex_key is None:
        raise ValueError("No key provided.")
    if len(hex_key) != 64:
        raise ValueError("Key must be 64 hex characters (256 bits).")
    try:
        key = bytes.fromhex(hex_key)
    except ValueError:
        raise ValueError("Key must be valid hex.")
    if len(key) != 32:
        raise ValueError("Decoded key is not 32 bytes.")
    return key

def encrypt_file(in_path: str, out_path: str, key: bytes):
    # Read whole file (binary)
    with open(in_path, "rb") as f:
        plaintext = f.read()

    # PKCS7 padding (block size 128 bits for AES)
    padder = padding.PKCS7(128).padder()
    padded = padder.update(plaintext) + padder.finalize()

    # Generate IV
    iv = os.urandom(16)

    # Create AES-CBC cipher
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(padded) + encryptor.finalize()

# Write IV || ciphertext (IV is needed for decryption)
    with open(out_path, "wb") as f:
        f.write(iv + ciphertext)

def main():
    args = parse_args()

    if args.generate_key:
        print(gen_key_hex())
        sys.exit(0)

    if not args.infile or not args.outfile or not args.key:
        print("Error: --infile, --outfile and --key are required (unless --generate-key).", file=sys.stderr)
        print(__doc__, file=sys.stderr)
        sys.exit(2)

    try:
        key = validate_and_decode_key(args.key)
    except ValueError as e:
        print(f"Key error: {e}", file=sys.stderr)
        sys.exit(2)

    if not os.path.isfile(args.infile):
        print(f"Input file not found: {args.infile}", file=sys.stderr)
        sys.exit(2)

    encrypt_file(args.infile, args.outfile, key)
    print(f"Encrypted {args.infile} -> {args.outfile} (IV prepended).")

if __name__ == "__main__":
    main()