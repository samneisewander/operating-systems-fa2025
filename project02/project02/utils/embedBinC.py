#!/usr/bin/env python3
import sys
import os

def file_to_c_array(input_path, output_path=None, var_name=None):
    # Determine output file name if not provided
    if output_path is None:
        output_path = os.path.splitext(input_path)[0] + "_data.c"

    # Determine variable name if not provided
    if var_name is None:
        var_name = os.path.splitext(os.path.basename(input_path))[0] + "_data"
        var_name = var_name.replace("-", "_").replace(".", "_")

    with open(input_path, "rb") as f:
        data = f.read()

    # Convert to comma-separated hex bytes
    byte_lines = []
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        line = ", ".join(f"0x{b:02X}" for b in chunk)
        byte_lines.append("    " + line + ",")

    array_str = "\n".join(byte_lines)

    c_code = f"""\
const unsigned char {var_name}[] = {{
{array_str}
}};

const unsigned char {var_name}_len = {len(data)};
"""

    with open(output_path, "w") as out:
        out.write(c_code)

    print(f"[+] Wrote {len(data)} bytes to {output_path} as variable '{var_name}'")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python file_to_c_array.py <input_file> [output_file] [var_name]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    variable_name = sys.argv[3] if len(sys.argv) > 3 else None

    file_to_c_array(input_file, output_file, variable_name)
