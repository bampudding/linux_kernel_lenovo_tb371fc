#!/usr/bin/env python3
"""Compare a vendor .ko set against one complete kernel Module.symvers.

Requires pyelftools. Read-only; does not load, sign, patch, or flash modules.
"""
import argparse
from collections import Counter
from pathlib import Path
from sys import stderr

try:
    from elftools.elf.elffile import ELFFile
except ImportError:
    raise SystemExit("pyelftools is required: python3 -m pip install pyelftools")


def read_exports(path):
    exports = {}
    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        fields = line.split("\t")
        if len(fields) < 2:
            raise ValueError(f"invalid Module.symvers line {line_number}")
        exports.setdefault(fields[1], set()).add(int(fields[0], 16))
    return exports


def read_imports(path):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        section = elf.get_section_by_name("__versions")
        if section is None:
            raise ValueError(f"{path}: missing __versions: CONFIG_MODVERSIONS not verified")
        pointer_size = elf.elfclass // 8
        item_size = pointer_size + 56  # Linux struct modversion_info / MODULE_NAME_LEN
        blob = section.data()
        if len(blob) % item_size:
            raise ValueError(f"{path}: unusual __versions layout ({len(blob)} bytes)")
        for index in range(0, len(blob), item_size):
            entry = blob[index:index + item_size]
            name = entry[pointer_size:].split(b"\0", 1)[0].decode("ascii", "replace")
            if name:
                crc = int.from_bytes(entry[:pointer_size], "little") & 0xffffffff
                yield name, crc


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("symvers", type=Path, help="Complete Module.symvers after 'make modules'")
    parser.add_argument("module_directory", type=Path, help="Directory containing .ko files")
    parser.add_argument("--report-only", action="store_true", help="Do not fail when mismatches exist")
    args = parser.parse_args()
    exports = read_exports(args.symvers)
    modules = sorted(args.module_directory.rglob("*.ko"))
    if not modules:
        parser.error("no modules found; cannot verify ABI")
    counts = Counter()
    missing = []
    mismatches = []
    for module in modules:
        for symbol, actual_crc in read_imports(module):
            counts["imports"] += 1
            accepted = exports.get(symbol)
            if accepted is None:
                missing.append((str(module.name), symbol, f"{actual_crc:08x}"))
                counts[module.name] += 1
            elif actual_crc not in accepted:
                mismatches.append((str(module.name), symbol, f"{actual_crc:08x}",
                                   ",".join(f"{candidate:08x}" for candidate in sorted(accepted))))
                counts[module.name] += 1
    print(f"Modules: {len(modules)} | versioned imports: {counts['imports']} | "
          f"missing symbols: {len(missing)} | mismatched CRCs: {len(mismatches)}")
    for name, number in sorted(((k, n) for k, n in counts.items() if k != "imports"),
                               key=lambda x: (-x[1], x[0])):
        print(f"  FAIL {name}: {number} unresolved or incompatible symbol references")
    for module, symbol, actual, expected in mismatches[:12]:
        print(f"    {module}::{symbol} {actual} != {expected}")
    for module, symbol, actual in missing[:12]:
        print(f"    {module}::{symbol} {actual} is not exported")
    if (mismatches or missing) and not args.report_only:
        print("INCOMPATIBLE: do not install this module set with the checked kernel", file=stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
