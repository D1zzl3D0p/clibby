"""Build-time texture conversion must not ship EXIF into Qt textures."""
from pathlib import Path
import struct
import tempfile
import unittest

from tools.convert_werewolf_qt import convert as convert_werewolf
from tools.convert_raven_qt import convert as convert_raven


ROOT = Path(__file__).resolve().parents[1]
BALSAM = ROOT / '.deps/qt/6.10.3/macos/bin/balsam'


def chunks(path):
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    offset = 8
    result = []
    while offset < len(data):
        size = struct.unpack_from('>I', data, offset)[0]
        result.append((data[offset+4:offset+8], data[offset:offset+size+12]))
        offset += size + 12
    return result


class TextureMetadataTests(unittest.TestCase):
    @unittest.skipUnless(BALSAM.is_file(), 'local Qt asset converter unavailable')
    def test_converted_textures_omit_exif_and_preserve_image_chunks(self):
        for name, convert in (('werewolf', convert_werewolf), ('raven', convert_raven)):
            with self.subTest(character=name), tempfile.TemporaryDirectory() as folder:
                source = ROOT / f'characters/{name}-3d'
                convert(source / f'{name}.glb', folder, BALSAM)
                texture = Path(folder) / 'maps/textureData.png'
                converted = chunks(texture)
                self.assertNotIn(b'eXIf', [kind for kind, _ in converted])
                # All pixel, palette, color and transparency data stay byte exact.
                self.assertEqual(converted, [(kind, raw) for kind, raw in
                                            chunks(source / 'atlas.png') if kind != b'eXIf'])
