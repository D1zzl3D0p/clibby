"""Deterministic motion studies, not a resident-controller or package API.

World coordinates are metres-like authoring units, Z up. The finite eight-second
demonstration has quiet rest before one second and after six seconds. Reading a
pose has no side effects. Continuous support is not represented as invented feet.
"""
from dataclasses import dataclass
import math

PETS = ("pebblekin", "margin", "lantern-snail", "velvet")
CAPABILITIES = {
    "pebblekin": frozenset(("anchored-support", "upper-stone-pivot")),
    "margin": frozenset(("bounded-morph", "comma", "exclamation")),
    "lantern-snail": frozenset(("continuous-support", "crawl", "peek", "door")),
    "velvet": frozenset(("six-contact-walk", "wing-reveal")),
}


@dataclass(frozen=True)
class Contact:
    name: str
    position: tuple
    planted: bool


@dataclass(frozen=True)
class Pose:
    pet: str
    root: tuple
    active: bool
    support_kind: str
    support_polygon: tuple
    contacts: tuple = ()
    points: tuple = ()
    values: tuple = ()

    def point(self, name):
        return dict(self.points)[name]

    def value(self, name):
        return dict(self.values)[name]


def smooth(u):
    u = min(1.0, max(0.0, u))
    return u * u * (3.0 - 2.0 * u)


def pulse(t, start=1.0, end=6.0):
    if t <= start or t >= end:
        return 0.0
    return math.sin(math.pi * (t - start) / (end - start)) ** 2


def rectangle(x, half_x, half_y):
    return tuple((x + a * half_x, b * half_y, 0.0)
                 for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1)))


def pebble(t):
    # Rotation about the upper stone's contact pivot; the base never travels.
    envelope = pulse(t)
    yaw = 0.42 * envelope
    wobble = 0.14 * envelope * math.sin(2.0 * math.pi * (t - 1.0))
    return Pose("pebblekin", (0.0, 0.0, 0.0), 1.0 < t < 6.0,
                "anchored", tuple((x, y, 0.0) for x, y in
                                  ((-.68,-.20),(-.40,-.40),(.40,-.40),(.68,-.20),
                                   (.68,.20),(.40,.40),(-.40,.40),(-.68,.20))),
                points=(("stoneContact", (.06, 0.0, 1.13)),),
                values=(("yaw", yaw), ("wobble", wobble)))


def pebble_geometry(pose):
    """Actual mesh and rolling contact, shared by renderer and geometry tests.

    A small flat upper-stone sole rocks on the middle stone's flat support face.
    Exact minimum-height correction keeps its lowest mesh vertex on that face;
    the contact moves within the face as the bounded rocking angle changes.
    """
    anchor = pose.point("stoneContact")

    def rings(levels):
        vertices = tuple((rx*math.cos(math.tau*i/8), ry*math.sin(math.tau*i/8), z)
                         for rx,ry,z in levels for i in range(8))
        faces = [tuple(range(7,-1,-1))]
        for level in range(len(levels)-1):
            for i in range(8):
                a, b = level*8+i, level*8+(i+1)%8
                faces.append((a,b,b+8,a+8))
        faces.append(tuple(range((len(levels)-1)*8, len(levels)*8)))
        return vertices, tuple(faces)

    middle_local, middle_faces = rings(((.20,.16,-.65), (.61,.42,-.30), (.24,.24,0.0)))
    middle = tuple((anchor[0]+x,anchor[1]+y,anchor[2]+z) for x,y,z in middle_local)
    upper_local, upper_faces = rings(((.16,.12,0.0), (.46,.34,.22), (.27,.22,.55)))
    yaw, wobble = pose.value("yaw"), pose.value("wobble")
    rotated = []
    for x,y,z in upper_local:
        x,z = x*math.cos(wobble)+z*math.sin(wobble), -x*math.sin(wobble)+z*math.cos(wobble)
        x,y = x*math.cos(yaw)-y*math.sin(yaw), x*math.sin(yaw)+y*math.cos(yaw)
        rotated.append((x,y,z))
    floor = min(p[2] for p in rotated)
    upper = tuple((anchor[0]+x, anchor[1]+y, anchor[2]+(z-floor)) for x,y,z in rotated)
    pivot = min(upper, key=lambda p:p[2])
    return {"middle": (middle,middle_faces), "upper": (upper,upper_faces),
            "pivot": pivot, "anchor": anchor, "lift": -floor}


def snail(t):
    travel = .8 * smooth((t - 1.0) / 4.0) - .4
    peek = pulse(t, 1.0, 6.0)
    # The lower surface remains on Z=0. Only the upper centreline ripples.
    curve = tuple((travel + (-1.0 + i / 12.0), 0.0,
                   .13 + .055 * peek * math.sin(i * .46 - t * 3.0))
                  for i in range(25))
    head = (travel + .81 + .25 * peek, 0.0, .30 + .19 * peek)
    lower = tuple((travel - 1.0 + i/12.0,
                   -.055 - .21 * math.sin(math.pi*i/24) ** .55, 0.0)
                  for i in range(25))
    outline = lower + tuple((x, -y, z) for x, y, z in reversed(lower))
    return Pose("lantern-snail", (travel, 0.0, 0.0), 1.0 < t < 6.0,
                "continuous", outline,
                points=tuple((f"body{i}", p) for i, p in enumerate(curve)) +
                       (("head", head),),
                values=(("peek", peek), ("door", 1.10 * peek)))


def margin(t):
    # Corresponding contour samples permit continuous, bounded deformation.
    comma = pulse(t, 1.0, 3.5)
    exclaim = pulse(t, 3.5, 6.0)
    vertices = []
    for i in range(32):
        a = 2.0 * math.pi * i / 32.0
        lower = max(0.0, -math.sin(a)) ** 3
        base_x = .60 * math.cos(a) * (1.0 + .22 * math.sin(a)) + .12 * math.sin(2*a)
        base_z = .32 + .78 * (1.0 + math.sin(a))
        comma_x = .60 * math.cos(a) + .70 * lower
        comma_z = .28 + .75 * (1.0 + math.sin(a))
        exclaim_x = .27 * math.cos(a) * (.78 + .25 * math.sin(a))
        exclaim_z = .46 + .88 * (1.0 + math.sin(a))
        x = base_x * (1-comma-exclaim) + comma_x*comma + exclaim_x*exclaim
        z = base_z * (1-comma-exclaim) + comma_z*comma + exclaim_z*exclaim
        vertices.append((x, 0.0, z))
    return Pose("margin", (0.0, 0.0, 0.0), 1.0 < t < 6.0,
                "bounded-body", rectangle(0.0, .45, .14),
                points=tuple((f"contour{i}", p) for i, p in enumerate(vertices)),
                values=(("comma", comma), ("exclamation", exclaim)))


def velvet(t):
    # Four finite cycles. Each tripod advances only while the other stays
    # planted in world space. No sinusoidal decorative leg animation.
    u = min(4.0, max(0.0, t - 1.0))
    root_x = -.4 + .2 * u
    cycle = min(3, int(u))
    local = u - cycle
    contacts, points = [], []
    for row, local_x in enumerate((.62, 0.0, -.62)):
        for side in (-1, 1):
            name = f"{'front' if row == 0 else 'middle' if row == 1 else 'hind'}{'Left' if side == -1 else 'Right'}"
            group = (row + (side == 1)) % 2
            start = .05 if group == 0 else .55
            end = start + .35
            fraction = smooth((local - start) / (end - start))
            planted = not (start < local < end)
            x = local_x - .4 + .2 * (cycle + fraction)
            z = .19 * math.sin(math.pi * fraction) if not planted else 0.0
            foot = (x, side * .64, z)
            hip = (root_x + local_x * .72, side * .19, .45)
            knee = ((hip[0] + x) * .5 - .10, side * .47, .38 + z * .5)
            contacts.append(Contact(name, foot, planted))
            points.extend(((name + "Hip", hip), (name + "Knee", knee)))
    polygon = tuple(c.position for c in contacts if c.planted)
    return Pose("velvet", (root_x, 0.0, 0.0), 1.0 < t < 6.0,
                "six-contact", polygon, tuple(contacts), tuple(points),
                (("wing", .78 * pulse(t, 4.7, 6.0)),))


def evaluate(pet, time):
    """Return an immutable pose; negative/nonfinite time and unknown pets reject."""
    if not math.isfinite(time) or time < 0:
        raise ValueError("time must be finite and nonnegative")
    if pet not in PETS:
        raise ValueError("unsupported experimental family")
    return {"pebblekin": pebble, "margin": margin,
            "lantern-snail": snail, "velvet": velvet}[pet](min(time, 6.0))
