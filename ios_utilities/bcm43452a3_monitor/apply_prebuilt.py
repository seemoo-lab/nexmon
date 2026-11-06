# SPDX-License-Identifier: GPL-3.0-only
"""Apply the pinned own-code patch to an owner's exact stock firmware, offline."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
STOCK = '280bc2d7ad25f2c789ce332c37da9d687693fbda156830803fe1516e8d938244'
RESULT = '0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def apply(stock, payload, metadata):
    if len(stock) != 527400 or sha(stock) != STOCK:
        raise ValueError('Stock firmware size/hash mismatch')
    if (metadata['schema'] != 1 or metadata['stock_sha256'] != STOCK
            or metadata['result_sha256'] != RESULT
            or len(payload) != metadata['payload_bytes']
            or sha(payload) != metadata['payload_sha256']):
        raise ValueError('Patch identity mismatch')
    result = bytearray(stock)
    used = []
    cursor = 0
    for row in metadata['ranges']:
        offset, length, position = row['offset'], row['length'], row['payload_offset']
        if any(type(value) is not int for value in (offset, length, position)):
            raise ValueError('Invalid range type')
        end = offset + length
        if (offset < 0 or length <= 0 or end > len(stock)
                or position != cursor or cursor + length > len(payload)
                or any(offset < b and a < end for a, b in used)):
            raise ValueError('Invalid or overlapping patch range')
        result[offset:end] = payload[cursor:cursor + length]
        used.append((offset, end))
        cursor += length
    if cursor != len(payload) or sha(result) != RESULT:
        raise ValueError('Patched firmware hash mismatch')
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stock', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    metadata = json.loads((ROOT / 'prebuilt/firmware.patch.json').read_text(encoding='utf-8'))
    result = apply(args.stock.read_bytes(), (ROOT / 'prebuilt/firmware.patch.bin').read_bytes(), metadata)
    # Exclusive creation: never overwrite the stock file or an existing candidate.
    with args.output.open('xb') as stream:
        stream.write(result)
    print('PASS: exact firmware generated; SHA256:', sha(result))


if __name__ == '__main__':
    main()
