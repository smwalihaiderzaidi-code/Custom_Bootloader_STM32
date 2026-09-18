import sys
import struct
import zlib
from pathlib import Path

HEADER_SIZE = 0x800

VERSION_OFFSET = 0x00
IMAGE_SIZE_OFFSET = 0x04
MAGIC_OFFSET = 0x08
IMAGE_CRC_OFFSET = 0x0C

EXPECTED_MAGIC = 0x50505050


def main():
    if len(sys.argv) != 2:
        print("Usage: python patch_crc.py <application.bin>")
        return 1

    binary_path = Path(sys.argv[1])

    if not binary_path.exists():
        print(f"ERROR: File not found: {binary_path}")
        return 1

    image = bytearray(binary_path.read_bytes())

    if len(image) <= HEADER_SIZE:
        print("ERROR: Binary does not contain application data")
        return 1

    version = struct.unpack_from("<I", image, VERSION_OFFSET)[0]
    magic = struct.unpack_from("<I", image, MAGIC_OFFSET)[0]

    if magic != EXPECTED_MAGIC:
        print(
            f"ERROR: Invalid magic: 0x{magic:08X}, "
            f"expected: 0x{EXPECTED_MAGIC:08X}"
        )
        return 1

    # Skip the complete 256-byte header.
    # CRC includes vector table and remaining application image.
    application_data = image[HEADER_SIZE:]

    image_size = len(application_data)
    image_crc = zlib.crc32(application_data) & 0xFFFFFFFF

    # Patch size and CRC into their actual structure offsets.
    struct.pack_into("<I", image, IMAGE_SIZE_OFFSET, image_size)
    struct.pack_into("<I", image, IMAGE_CRC_OFFSET, image_crc)

    binary_path.write_bytes(image)

    print(f"Version    : {version}")
    print(f"Magic      : 0x{magic:08X}")
    print(f"Image size : {image_size} bytes")
    print(f"Image CRC  : 0x{image_crc:08X}")
    print(f"Patched    : {binary_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())