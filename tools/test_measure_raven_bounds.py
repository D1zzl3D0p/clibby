"""Regression for skin triangles spanning different vertex influence sets."""
import json
from pathlib import Path
import struct
import tempfile
import unittest

from measure_raven_bounds import measure


class TriangleCoverage(unittest.TestCase):
    def test_cross_influence_triangle_includes_all_vertices_in_both_joint_boxes(self):
        data = dict(nodes=[dict(name="a"), dict(name="b"), dict(mesh=0)],
                    accessors=[], bufferViews=[], skins=[], meshes=[])
        binary = bytearray()

        def accessor(values, kind, component, fmt):
            while len(binary) % 4:
                binary.append(0)
            offset = len(binary)
            for value in values:
                binary.extend(struct.pack("<" + fmt, *value))
            view = len(data["bufferViews"])
            data["bufferViews"].append(dict(buffer=0, byteOffset=offset,
                                            byteLength=len(binary) - offset))
            result = len(data["accessors"])
            data["accessors"].append(dict(bufferView=view, componentType=component,
                                          count=len(values), type=kind))
            return result

        positions = accessor([(0, 0, 0), (2, 0, 0), (0, 2, 0)], "VEC3", 5126, "3f")
        joints = accessor([(0, 0, 0, 0), (0, 1, 0, 0), (1, 0, 0, 0)], "VEC4", 5123, "4H")
        weights = accessor([(1, 0, 0, 0), (.5, .5, 0, 0), (1, 0, 0, 0)], "VEC4", 5126, "4f")
        indices = accessor([(0,), (1,), (2,)], "SCALAR", 5123, "H")
        identity = (1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
        inverse = accessor([identity, identity], "MAT4", 5126, "16f")
        data["skins"] = [dict(joints=[0, 1], inverseBindMatrices=inverse)]
        data["meshes"] = [dict(primitives=[dict(indices=indices,
            attributes=dict(POSITION=positions, JOINTS_0=joints, WEIGHTS_0=weights))])]
        encoded = json.dumps(data).encode()
        encoded += b" " * (-len(encoded) % 4)
        binary += b"\0" * (-len(binary) % 4)
        raw = struct.pack("<III", 0x46546C67, 2, 28 + len(encoded) + len(binary))
        raw += struct.pack("<II", len(encoded), 0x4E4F534A) + encoded
        raw += struct.pack("<II", len(binary), 0x004E4942) + binary
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "triangle.glb"
            path.write_bytes(raw)
            groups = measure(path)
        self.assertEqual(len(groups), 1, "one face needs one union-of-influences hull")
        self.assertEqual({box["joint"] for box in groups[0]}, {"a", "b"})
        for box in groups[0]:
            self.assertEqual(box["minimum"], [0, 0, 0])
            self.assertEqual(box["maximum"], [2, 2, 0])


if __name__ == "__main__":
    unittest.main()
