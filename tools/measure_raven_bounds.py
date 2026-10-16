"""Measure conservative skin influence bounds from our exported, trusted GLB."""
import argparse
import collections
import json
import math
from pathlib import Path
import struct


def measure(path):
    raw = Path(path).read_bytes()
    size = struct.unpack_from("<I", raw, 12)[0]
    data = json.loads(raw[20:20 + size])
    binary = 28 + size
    formats = {5121: "B", 5123: "H", 5125: "I", 5126: "f"}
    dimensions = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}

    def accessor(index):
        value = data["accessors"][index]
        view = data["bufferViews"][value["bufferView"]]
        fmt = "<" + formats[value["componentType"]] * dimensions[value["type"]]
        width = struct.calcsize(fmt)
        offset = binary + view.get("byteOffset", 0) + value.get("byteOffset", 0)
        return [struct.unpack_from(fmt, raw, offset + n * view.get("byteStride", width))
                for n in range(value["count"])]

    skin = data["skins"][0]
    names = [data["nodes"][n]["name"] for n in skin["joints"]]
    inverse = accessor(skin["inverseBindMatrices"])
    groups = collections.defaultdict(lambda: collections.defaultdict(list))
    for node in data["nodes"]:
        if "mesh" not in node:
            continue
        # Current original authoring applies all mesh transforms before export.
        assert not any(k in node for k in ("matrix", "translation", "rotation", "scale"))
        for primitive in data["meshes"][node["mesh"]]["primitives"]:
            attributes = primitive["attributes"]
            positions = accessor(attributes["POSITION"])
            joints = accessor(attributes["JOINTS_0"])
            weights = accessor(attributes["WEIGHTS_0"])
            indices = [entry[0] for entry in accessor(primitive["indices"])]
            assert primitive.get("mode", 4) == 4 and len(indices) % 3 == 0
            for start in range(0, len(indices), 3):
                face = indices[start:start + 3]
                # A rasterized triangle interpolates all three skinned vertices.
                # Its conservative hull therefore needs the UNION of the face's
                # influences, including singleton-to-blended seam transitions.
                active = tuple(sorted({joint for vertex in face
                    for joint, weight in zip(joints[vertex], weights[vertex]) if weight > 0}))
                for joint in active:
                    matrix = inverse[joint]
                    for vertex in face:
                        point = positions[vertex]
                        local = [sum(matrix[column * 4 + row] * coordinate
                                     for column, coordinate in enumerate((*point, 1)))
                                 for row in range(3)]
                        assert all(math.isfinite(v) and abs(v) < 10 for v in local)
                        groups[active][joint].append(local)
    result = []
    for active, influences in sorted(groups.items()):
        result.append([dict(joint=names[joint],
                            minimum=[min(p[axis] for p in points) for axis in range(3)],
                            maximum=[max(p[axis] for p in points) for axis in range(3)])
                       for joint, points in sorted(influences.items())])
    assert 0 < len(result) <= 64
    return result


def update(glb, rig):
    path = Path(rig)
    data = json.loads(path.read_text())
    # Each group is a triangle influence union. Every skinned face, including
    # its rasterized interior, lies inside its transformed influence hull.
    data["skinBounds"] = measure(glb)
    path.write_text(json.dumps(data, indent=2) + "\n")
    return len(data["skinBounds"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--glb", type=Path, required=True)
    parser.add_argument("--rig", type=Path, required=True)
    args = parser.parse_args()
    print(f"Measured {update(args.glb, args.rig)} skin influence groups")
