"""Remove camera metadata from trusted build-time PNG textures, losslessly."""
from pathlib import Path
import struct


def strip_texture_exif(path):
    path = Path(path)
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError(f'Not a PNG: {path}')
    parts = [data[:8]]
    offset = 8
    while offset < len(data):
        if offset + 12 > len(data):
            raise ValueError(f'Truncated PNG: {path}')
        size = struct.unpack_from('>I', data, offset)[0]
        end = offset + size + 12
        if end > len(data):
            raise ValueError(f'Truncated PNG chunk: {path}')
        # EXIF is unused by our UV-mapped textures. Preserve every other chunk,
        # including compressed pixels, color profiles, transparency and CRCs.
        if data[offset+4:offset+8] != b'eXIf':
            parts.append(data[offset:end])
        offset = end
    cleaned = b''.join(parts)
    if cleaned != data:
        path.write_bytes(cleaned)
