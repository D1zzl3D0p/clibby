#include "loader.h"
#include "fixtures.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pets::character;
static void check(bool v, const char *text) {
  if (!v)
    throw std::runtime_error(text);
}
template <class F> static void rejects(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  check(rejected, "expected rejection");
}
static Definition sample() {
  Definition d;
  d.id = "test";
  d.name = "Test";
  d.controller = "soft";
  d.bones = {{"tip", 1, {5, 0, 0, 1, 1}},
             {"trunk", -1, {40, 40, 0, 1, 1}},
             {"toe", 1, {0, 20, 0, 1, 1}}};
  d.bindings = {{"body", 1}, {"head", 0}, {"footLeft", 2}};
  d.contacts = {{2, {0, 0}}};
  d.attachments = {{0, {0, 0, 8, 8}, {4, 4}, {0, 0}, 2},
                   {1, {8, 0, 8, 8}, {4, 4}, {0, 0}, -5}};
  d.interaction = {{0, 0}, {96, 0}, {96, 96}, {0, 96}};
  return d;
}
static QByteArray manifest() {
  return R"({"version":1,"id":"test","name":"Test","controller":"soft","width":96,"height":96,"origin":[48,80],"atlas":"atlas.png","bones":[{"id":"tip","parent":"trunk","rest":{"x":5,"y":0}},{"id":"trunk","parent":null,"rest":{"x":40,"y":40}}],"attachments":[{"bone":"tip","source":[0,0,8,8],"pivot":[4,4],"offset":[0,0],"drawOrder":9}],"bindings":{"body":"trunk","head":"tip"},"interaction":[[0,0],[96,0],[96,96]],"contacts":[]})";
}
static void write(const QString &p, const QByteArray &data) {
  QFile f(p);
  check(f.open(QIODevice::WriteOnly), "write");
  check(f.write(data) == data.size(), "write data");
}
// A shuffled four-chain package catches loaders that preserve JSON ordering or
// silently infer a biped from an omitted gait.
static QJsonObject quadrupedManifest() {
  auto doc = QJsonDocument::fromJson(manifest()).object();
  QJsonArray bones{QJsonObject{{"id", "body"}, {"parent", QJsonValue::Null},
                             {"rest", QJsonObject{{"x", 48}, {"y", 20}}}}};
  QJsonArray legs;
  for (int i : {3, 0, 2, 1}) {
    const QString prefix = QString("leg%1").arg(i);
    for (int j = 0; j < 3; ++j) {
      bones.append(QJsonObject{
          {"id", prefix + QString::number(j)},
          {"parent", j == 0 ? QString("body") : prefix + QString::number(j - 1)},
          {"rest", QJsonObject{{"x", j == 0 ? -18 + i * 12 : 0},
                               {"y", j == 0 ? 0 : 12}}}});
    }
    legs.append(QJsonObject{
        {"role", QStringList{"foreLeft", "foreRight", "hindLeft", "hindRight"}[i]},
        {"hip", prefix + "0"}, {"knee", prefix + "1"}, {"ankle", prefix + "2"},
        {"contact", QJsonArray{0, 0}}, {"bend", i % 2 ? -1 : 1}});
  }
  doc["bones"] = bones;
  doc["bindings"] = QJsonObject{{"body", "body"}};
  doc["attachments"] = QJsonArray{QJsonObject{
      {"bone", "body"}, {"source", QJsonArray{0, 0, 8, 8}},
      {"pivot", QJsonArray{4, 4}}, {"offset", QJsonArray{0, 0}}, {"drawOrder", 0}}};
  doc["locomotion"] = QJsonObject{
      {"gait", "quadrupedWalk"}, {"groundY", 44}, {"legs", legs},
      {"reachThreshold", 6}, {"stride", 8}, {"lift", 4}};
  return doc;
}
static void boundedLimbs() {
  LimbArray<int> limbs;
  check(limbs.size() == 2 && limbs[0] == 0 && limbs[1] == 0, "default two active limbs");
  limbs = {3, 7}; limbs.setCount(4);
  check(limbs[0] == 3 && limbs[1] == 7 && limbs[2] == 0 && limbs[3] == 0,
        "activating limbs initializes only new elements");
  limbs.fill(9);
  auto copy = limbs;
  check(copy.size() == 4 && copy.array<4>()[3] == 9, "copy retains four active limbs");
  rejects([&] { copy.array<2>(); });
  limbs.setCount(2);
  int visits = 0; for (auto value : limbs) { check(value == 9, "active value"); ++visits; }
  check(visits == 2, "spare limbs never iterate");
  rejects([&] { (void)limbs[2]; });
  limbs.setCount(4);
  check(limbs[2] == 0 && limbs[3] == 0, "reactivation clears old spare values");
  for (std::size_t count : {0, 1, 3, 5, 6}) rejects([&] { limbs.setCount(count); });
  rejects([&] { limbs = {1, 2, 3}; });
  check(limbs.size() == 4, "invalid count leaves active anatomy intact");
  limbs = std::array<int, 2>{{4, 8}};
  check(limbs.array<2>()[1] == 8, "legacy array assignment");
  limbs = std::array<int, 4>{{1, 2, 3, 4}};
  check(limbs.size() == 4 && limbs[3] == 4, "four array assignment");
  LimbArray<int> nested{{5, 6}};
  check(nested.size() == 2 && nested[1] == 6, "nested legacy array initializer");
  const LimbArray<int> frozen{1, 2, 3, 4};
  check(frozen.end() - frozen.begin() == 4 && frozen[3] == 4, "const iteration active range");
}
static void quadrupedPackages() {
  QTemporaryDir dir;
  check(dir.isValid(), "quad temp dir");
  QImage atlas(16, 16, QImage::Format_ARGB32);
  atlas.fill(Qt::white);
  check(atlas.save(dir.path() + "/atlas.png"), "quad atlas");
  auto load = [&](QJsonObject doc) {
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    return loadCharacter(dir.path()).definition;
  };
  auto doc = quadrupedManifest();
  const auto quad = load(doc);
  check(quad.locomotion->legs.size() == 4, "four active chains load");
  for (int i = 0; i < 4; ++i)
    check(quad.bones[quad.locomotion->legs[i].hip].id == "leg" + std::to_string(i) + "0",
          "roles normalize shuffled chains");
  for (const double value : {std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()}) {
    auto invalid = quad;
    invalid.locomotion->secondary.frequency = value;
    rejects([&] { validateDefinition(invalid); });
    invalid = quad; invalid.locomotion->speed = value;
    rejects([&] { validateDefinition(invalid); });
  }
  auto mismatch = quad;
  mismatch.locomotion->gait = Gait::Biped;
  rejects([&] { validateDefinition(mismatch); });
  mismatch = quad;
  mismatch.locomotion->gait = static_cast<Gait>(9);
  rejects([&] { validateDefinition(mismatch); });
  mismatch = quad;
  mismatch.locomotion->legs.setCount(2);
  rejects([&] { validateDefinition(mismatch); });
  auto bad = doc;
  auto locomotion = bad["locomotion"].toObject();
  locomotion.remove("gait");
  bad["locomotion"] = locomotion;
  rejects([&] { load(bad); });
  for (const QString gait : {"biped", "unknown"}) {
    locomotion = doc["locomotion"].toObject();
    locomotion["gait"] = gait;
    bad = doc; bad["locomotion"] = locomotion;
    rejects([&] { load(bad); });
  }
  for (int count : {0, 1, 2, 3, 5, 6}) {
    locomotion = doc["locomotion"].toObject();
    auto malformed = locomotion["legs"].toArray();
    while (malformed.size() > count) malformed.removeLast();
    while (malformed.size() < count) malformed.append(malformed[0]);
    locomotion["legs"] = malformed; bad = doc; bad["locomotion"] = locomotion;
    rejects([&] { load(bad); });
  }
  for (int problem = 0; problem < 5; ++problem) {
    locomotion = doc["locomotion"].toObject();
    auto legs = locomotion["legs"].toArray();
    auto chain = legs[0].toObject();
    if (problem == 0) chain.remove("role");
    if (problem == 1) chain["role"] = "foreLeft";
    if (problem == 2) chain["role"] = "unknown";
    if (problem == 3) chain.remove("knee");
    if (problem == 4) chain["hip"] = legs[1].toObject()["hip"];
    legs[0] = chain; locomotion["legs"] = legs;
    bad = doc; bad["locomotion"] = locomotion;
    rejects([&] { load(bad); });
  }
  // Distinct joint IDs can still overlap through ancestry.
  bad = doc;
  auto bones = bad["bones"].toArray();
  auto hip = bones[4].toObject();
  hip["parent"] = "leg32";
  hip["rest"] = QJsonObject{{"x", -36}, {"y", -24}};
  bones[4] = hip; bad["bones"] = bones;
  rejects([&] { load(bad); });
  // Explicit biped roles stay ordered and old manifests need no roles/gait.
  locomotion = doc["locomotion"].toObject();
  auto legs = locomotion["legs"].toArray();
  while (legs.size() > 2) legs.removeLast();
  for (int i = 0; i < 2; ++i) {
    auto chain = legs[i].toObject();
    chain["role"] = i == 0 ? "left" : "right"; legs[i] = chain;
  }
  locomotion["legs"] = legs; locomotion["gait"] = "biped";
  bad = doc; bad["locomotion"] = locomotion;
  check(load(bad).locomotion->legs.size() == 2, "explicit biped loads");
  auto wrong = legs[0].toObject(); wrong["role"] = "right"; legs[0] = wrong;
  locomotion["legs"] = legs; bad["locomotion"] = locomotion;
  rejects([&] { load(bad); });
  locomotion.remove("gait");
  for (int i = 0; i < 2; ++i) {
    auto chain = legs[i].toObject(); chain.remove("role"); legs[i] = chain;
  }
  locomotion["legs"] = legs; bad["locomotion"] = locomotion;
  check(load(bad).locomotion->legs.size() == 2, "legacy biped loads");
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    boundedLimbs();
    quadrupedPackages();
    auto d = sample();
    validateDefinition(d);
    Controller c(d);
    auto p = c.pose(Action::Rest, 0);
    auto w = worldTransforms(d, p);
    check(w[0].tx == 45 && w[0].ty == 40, "parent-after-child transform");
    auto anchors = utilityAnchors(d, p);
    check(anchors.head.x == 45 && anchors.head.y == 40, "head anchor follows binding");
    check(anchors.card.y < anchors.head.y && anchors.effect.y < anchors.head.y,
          "card and effect anchors clear head art");
    p.bones[0].x += 3;
    check(utilityAnchors(d, p).head.x == 48, "anchor tracks posed head");
    check(!anchors.leftGrip && !anchors.rightGrip, "armless fixture has no fake grips");
    check(gestureAction(Gesture::Wave) == Action::Attend &&
              gestureAction(Gesture::Nod) == Action::Acknowledge,
          "semantic gestures have armless fallbacks");
    check(gestureEnvelope(1.0) == 0 && gestureEnvelope(0.5) > .99,
          "gesture settles without repeated motion");
    check(d.attachments[0].drawOrder > d.attachments[1].drawOrder,
          "independent layering");
    auto bad = d;
    bad.bones[1].parent = 0;
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.bones[0].rest.x = std::numeric_limits<double>::infinity();
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.bindings.erase("body");
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.controller = "executable";
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.id = "../test";
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.attachments[0].bone = 99;
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.attachments.clear();
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.interaction[0].x = -1;
    rejects([&] { validateDefinition(bad); });
    bad = d;
    bad.bones.resize(65);
    rejects([&] { validateDefinition(bad); });
    auto minimal = d;
    minimal.bindings = {{"body", 1}};
    Controller optional(minimal);
    check(optional.pose(Action::Attend, 0).bones.size() == 3,
          "missing optional capabilities supported");
    auto scaleLimit = d;
    scaleLimit.bones[1].rest.scaleY = .05;
    scaleLimit.contacts.clear();
    Controller scaleController(scaleLimit);
    worldTransforms(scaleLimit, scaleController.pose(Action::Idle, 2));
    auto coordinateLimit = d;
    coordinateLimit.contacts.clear();
    coordinateLimit.bones[0].rest.y = 8192;
    Controller coordinateController(coordinateLimit);
    coordinateController.impulse({0, 1});
    coordinateController.step(1.0 / 60);
    auto coordinatePose = coordinateController.pose(Action::Rest, 0);
    worldTransforms(coordinateLimit, coordinatePose);
    check(coordinatePose.bones[0].y <= 8192,
          "dynamic local coordinates stay bounded");
    auto compressed = d;
    compressed.bones.clear();
    for (int i = 0; i < 8; ++i)
      compressed.bones.push_back(
          {"compressed" + std::to_string(i), i - 1, {0, 1, 0, .05, .05}});
    compressed.attachments = {d.attachments[0]};
    compressed.bindings = {{"body", 0}, {"head", 7}};
    compressed.contacts = {{7, {0, 1}}};
    Controller compressedController(compressed);
    compressedController.impulse({1, 1});
    compressedController.step(1.0 / 60);
    auto compressedPose = compressedController.pose(Action::Idle, .5);
    auto compressedWorld = worldTransforms(compressed, compressedPose);
    Pose compressedRest;
    for (auto b : compressed.bones)
      compressedRest.bones.push_back(b.rest);
    auto compressedGround =
        worldTransforms(compressed, compressedRest)[7].apply({0, 1});
    auto compressedActual = compressedWorld[7].apply({0, 1});
    check(std::abs(compressedActual.x - compressedGround.x) < 1e-9 &&
              std::abs(compressedActual.y - compressedGround.y) < 1e-9,
          "ill-conditioned contacts safely preserve rest ground");
    auto excessiveWorld = d;
    excessiveWorld.bones.clear();
    excessiveWorld.attachments = {d.attachments[0]};
    excessiveWorld.bindings = {{"body", 0}};
    excessiveWorld.contacts.clear();
    for (int i = 0; i < 32; ++i)
      excessiveWorld.bones.push_back(
          {"large" + std::to_string(i), i - 1, {0, 0, 0, 20, 20}});
    rejects([&] { validateDefinition(excessiveWorld); });
    Pose excessivePose;
    for (auto bone : excessiveWorld.bones)
      excessivePose.bones.push_back(bone.rest);
    rejects([&] { worldTransforms(excessiveWorld, excessivePose); });
    auto nearWorldLimit = excessiveWorld;
    nearWorldLimit.bones.resize(8);
    for (auto &bone : nearWorldLimit.bones)
      bone.rest.scaleX = bone.rest.scaleY = 10;
    nearWorldLimit.bones.back().rest.scaleX =
        nearWorldLimit.bones.back().rest.scaleY = 9.95;
    Controller nearWorldController(nearWorldLimit);
    auto boundedWorld = worldTransforms(
        nearWorldLimit, nearWorldController.pose(Action::Idle, .5));
    check(std::abs(boundedWorld.back().m11) <= 1e8,
          "accepted dynamic pose preserves world matrix limit");
    c.impulse({1, 1}, 1000);
    check(!c.settled(), "impulse starts dynamics");
    c.step(1e9);
    p = c.pose(Action::Idle, 1);
    w = worldTransforms(d, p);
    check(std::abs(w[2].ty - 60) < 1e-7, "grounded foot after impulse");
    check(std::abs(p.bones[1].rotation) <= .4, "bounded lean");
    for (int i = 0; i < 600; i++)
      c.step(1.0 / 60);
    check(c.settled(), "finite settling");
    c.impulse({1, 0});
    c.resetDynamics();
    check(c.settled(), "reset settles");
    rejects([&] { c.step(std::numeric_limits<double>::quiet_NaN()); });
    rejects([&] { c.impulse({INFINITY, 0}); });
    for (auto a : {Action::Rest, Action::Idle, Action::Walk, Action::Attend,
                   Action::Celebrate, Action::Failure, Action::Acknowledge}) {
      p = c.pose(a, 3, {20, 10});
      for (auto b : p.bones)
        check(std::isfinite(b.rotation), "finite action");
    }
    auto idle = c.pose(Action::Idle, 1);
    for (auto a : {Action::Celebrate, Action::Failure, Action::Acknowledge}) {
      auto end = c.pose(a, 1);
      for (size_t i = 0; i < end.bones.size(); ++i)
        check(std::abs(end.bones[i].rotation - idle.bones[i].rotation) < 1e-9 &&
                  std::abs(end.bones[i].scaleY - idle.bones[i].scaleY) < 1e-9,
              "reaction endpoint returns to idle");
    }
    QTemporaryDir dir;
    check(dir.isValid(), "temp dir");
    QImage image(16, 16, QImage::Format_ARGB32);
    image.fill(Qt::white);
    check(image.save(dir.path() + "/atlas.png"), "save image");
    write(dir.path() + "/manifest.json", manifest());
    auto loaded = loadCharacter(dir.path());
    check(loaded.definition.bones[0].parent == 1,
          "loader resolves arbitrary hierarchy");
    check(loaded.atlas.width() == 16, "atlas loaded");
    auto versionDoc = QJsonDocument::fromJson(manifest()).object();
    versionDoc.remove("version");
    write(dir.path() + "/manifest.json", QJsonDocument(versionDoc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    versionDoc["version"] = 2;
    write(dir.path() + "/manifest.json", QJsonDocument(versionDoc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    versionDoc["version"] = "1";
    write(dir.path() + "/manifest.json", QJsonDocument(versionDoc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    auto doc = QJsonDocument::fromJson(manifest()).object();
    doc["atlas"] = "../escape.png";
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    check(loaded.definition.id == "test",
          "failed selection preserves prior value");
    doc = QJsonDocument::fromJson(manifest()).object();
    doc["width"] = "96";
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    doc = QJsonDocument::fromJson(manifest()).object();
    auto at = doc["attachments"].toArray();
    auto a = at[0].toObject();
    a["source"] = QJsonArray{0, 0, 17, 8};
    at[0] = a;
    doc["attachments"] = at;
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    write(dir.path() + "/manifest.json", manifest());
    QImage huge(2049, 1, QImage::Format_ARGB32);
    huge.fill(Qt::white);
    huge.save(dir.path() + "/atlas.png");
    rejects([&] { loadCharacter(dir.path()); });
    write(dir.path() + "/manifest.json", QByteArray(1024 * 1024 + 1, ' '));
    rejects([&] { loadCharacter(dir.path()); });
    write(dir.path() + "/manifest.json", manifest());
    image.save(dir.path() + "/atlas.png");
    QTemporaryDir external;
    write(external.path() + "/outside.txt", "outside");
    check(QFile::link(external.path() + "/outside.txt",
                      dir.path() + "/escaped-link"),
          "create symlink");
    rejects([&] { loadCharacter(dir.path()); });
    QFile::remove(dir.path() + "/escaped-link");
    doc = QJsonDocument::fromJson(manifest()).object();
    auto bs = doc["bones"].toArray();
    auto b = bs[1].toObject();
    b["parent"] = "tip";
    bs[1] = b;
    doc["bones"] = bs;
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    doc = QJsonDocument::fromJson(manifest()).object();
    bs = doc["bones"].toArray();
    b = bs[0].toObject();
    b["parent"] = true;
    bs[0] = b;
    doc["bones"] = bs;
    write(dir.path() + "/manifest.json", QJsonDocument(doc).toJson());
    rejects([&] { loadCharacter(dir.path()); });
    write(dir.path() + "/manifest.json", manifest());
    QFile large(dir.path() + "/oversized.bin");
    check(large.open(QIODevice::WriteOnly) &&
              large.resize(32 * 1024 * 1024 + 1),
          "oversize fixture");
    large.close();
    rejects([&] { loadCharacter(dir.path()); });
    QFile::remove(large.fileName());
    QStringList examples;
    for (int i = 1; i < argc; i++) {
      QString path = QString::fromLocal8Bit(argv[i]);
      if (QFileInfo::exists(path + "/manifest.json"))
        examples << path;
      else
        for (auto child :
             QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
          if (QFileInfo::exists(path + "/" + child + "/manifest.json"))
            examples << path + "/" + child;
    }
    if (argc > 1)
      check(!examples.isEmpty(), "example package arguments found");
    for (auto path : examples) {
      QFile descriptor(path + "/manifest.json");
      check(descriptor.open(QIODevice::ReadOnly), "example manifest opens");
      const auto kind = QJsonDocument::fromJson(descriptor.readAll()).object().value("kind").toString();
      if (kind == "builtin-3d") {
        rejects([&] { loadCharacter(path); });
        continue; // 3D descriptors are tested by resident_spatial, never as 2D rigs.
      }
      auto example = loadCharacter(path);
      Controller controller(example.definition);
      for (auto a : {Action::Rest, Action::Idle, Action::Walk, Action::Attend,
                     Action::Celebrate, Action::Failure, Action::Acknowledge}) {
        auto pose = controller.pose(a, .5, {10, 5});
        auto world = worldTransforms(example.definition, pose);
        check(world.size() == example.definition.bones.size(),
              "example pose resolves");
      }
      std::cout << "Loaded example: " << example.definition.id << '\n';
    }
    std::cout << "Character hierarchy, loader, actions, contacts and spring "
                 "cases passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
