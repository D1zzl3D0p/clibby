#include "loader.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <cmath>
#include <map>
#include <stdexcept>
namespace pets::character {
namespace {
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
QJsonValue field(const QJsonObject &o, const char *key) {
  require(o.contains(key), "missing manifest field");
  return o.value(key);
}
QJsonObject object(QJsonValue v) {
  require(v.isObject(), "expected object");
  return v.toObject();
}
QJsonArray array(QJsonValue v, int limit = 128) {
  require(v.isArray(), "expected array");
  auto a = v.toArray();
  require(a.size() <= limit, "array size exceeded");
  return a;
}
double number(QJsonValue v) {
  require(v.isDouble() && std::isfinite(v.toDouble()),
          "expected finite number");
  return v.toDouble();
}
int integer(QJsonValue v) {
  double n = number(v);
  require(n == std::floor(n) && n >= -100000 && n <= 100000,
          "expected bounded integer");
  return int(n);
}
std::string string(QJsonValue v) {
  require(v.isString() && !v.toString().isEmpty() && v.toString().size() <= 128,
          "expected bounded string");
  return v.toString().toStdString();
}
Vec2 vec(QJsonValue v) {
  auto a = array(v, 2);
  require(a.size() == 2, "expected 2D point");
  return {number(a[0]), number(a[1])};
}
Transform transform(QJsonValue v) {
  auto o = object(v);
  Transform t;
  t.x = number(field(o, "x"));
  t.y = number(field(o, "y"));
  if (o.contains("rotation"))
    t.rotation = number(o["rotation"]);
  if (o.contains("scaleX"))
    t.scaleX = number(o["scaleX"]);
  if (o.contains("scaleY"))
    t.scaleY = number(o["scaleY"]);
  return t;
}
QString safeAsset(const QString &directory, const QString &relative) {
  require(!relative.isEmpty() && !QDir::isAbsolutePath(relative) &&
              !relative.contains('\\'),
          "invalid asset path");
  auto parts = relative.split('/');
  for (auto p : parts)
    require(!p.isEmpty() && p != "." && p != "..", "asset traversal");
  QString path = directory;
  for (auto p : parts) {
    path += "/" + p;
    require(!QFileInfo(path).isSymLink(), "symlink asset forbidden");
  }
  QFileInfo f(path);
  require(f.isFile() && f.canonicalFilePath().startsWith(directory + "/"),
          "asset escapes package or missing");
  return f.canonicalFilePath();
}
} // namespace
LoadedCharacter loadCharacter(const QString &directory) {
  QFileInfo root(directory);
  require(root.isDir() && !root.isSymLink(),
          "character directory missing or symlink");
  QString base = root.canonicalFilePath();
  require(!base.isEmpty(), "invalid character directory");
  // Inspect every entry before opening/decoding package content. Symlinks are
  // deliberately unsupported, including internal ones, to keep containment
  // simple.
  qint64 size = 0;
  int entries = 0;
  QDirIterator it(base,
                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                      QDir::System,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    auto info = it.fileInfo();
    require(++entries <= 1024, "too many package entries");
    require(!info.isSymLink(), "package symlink forbidden");
    require(info.isDir() || info.isFile(), "unsupported package entry");
    if (info.isFile()) {
      size += info.size();
      require(size <= 32 * 1024 * 1024, "package exceeds 32 MiB");
    }
  }
  QString manifest = safeAsset(base, "manifest.json");
  QFile file(manifest);
  require(file.size() <= 1024 * 1024, "manifest exceeds 1 MiB");
  require(file.open(QIODevice::ReadOnly), "cannot read manifest");
  auto bytes = file.read(1024 * 1024 + 1);
  require(bytes.size() <= 1024 * 1024, "manifest exceeds 1 MiB");
  QJsonParseError error;
  auto doc = QJsonDocument::fromJson(bytes, &error);
  require(error.error == QJsonParseError::NoError && doc.isObject(),
          "invalid manifest JSON");
  auto o = doc.object();
  require(integer(field(o, "version")) == 1, "unsupported manifest version");
  LoadedCharacter loaded;
  auto &d = loaded.definition;
  d.id = string(field(o, "id"));
  d.name = string(field(o, "name"));
  d.controller = string(field(o, "controller"));
  d.width = integer(field(o, "width"));
  d.height = integer(field(o, "height"));
  d.origin = vec(field(o, "origin"));
  if (o.contains("motionEnvelope"))
    d.motionEnvelope = number(o["motionEnvelope"]);
  auto bones = array(field(o, "bones"), 64);
  std::map<std::string, int> indices;
  std::vector<QJsonValue> parents;
  for (auto value : bones) {
    auto b = object(value);
    Bone bone;
    bone.id = string(field(b, "id"));
    require(indices.emplace(bone.id, int(d.bones.size())).second,
            "duplicate bone ID");
    bone.rest = transform(field(b, "rest"));
    if (b.contains("minRotation"))
      bone.minRotation = number(b["minRotation"]);
    if (b.contains("maxRotation"))
      bone.maxRotation = number(b["maxRotation"]);
    parents.push_back(field(b, "parent"));
    d.bones.push_back(bone);
  }
  auto index = [&](QJsonValue v) {
    auto id = string(v);
    auto i = indices.find(id);
    require(i != indices.end(), "unknown bone reference");
    return i->second;
  };
  for (size_t i = 0; i < parents.size(); ++i) {
    require(parents[i].isNull() || parents[i].isString(),
            "parent must be ID or null");
    d.bones[i].parent = parents[i].isNull() ? -1 : index(parents[i]);
  }
  for (auto value : array(field(o, "attachments"), 64)) {
    auto a = object(value);
    Attachment attachment;
    attachment.bone = index(field(a, "bone"));
    auto r = array(field(a, "source"), 4);
    require(r.size() == 4, "expected source rectangle");
    attachment.source = {integer(r[0]), integer(r[1]), integer(r[2]),
                         integer(r[3])};
    attachment.pivot = vec(field(a, "pivot"));
    attachment.offset = vec(field(a, "offset"));
    attachment.drawOrder = integer(field(a, "drawOrder"));
    d.attachments.push_back(attachment);
  }
  auto bindings = object(field(o, "bindings"));
  require(bindings.size() <= 64, "too many bindings");
  for (auto i = bindings.begin(); i != bindings.end(); ++i) {
    require(!i.key().isEmpty() && i.key().size() <= 128,
            "invalid binding name");
    d.bindings.emplace(i.key().toStdString(), index(i.value()));
  }
  for (auto v : array(field(o, "interaction")))
    d.interaction.push_back(vec(v));
  if (o.contains("contacts"))
    for (auto value : array(o["contacts"], 64)) {
      auto c = object(value);
      d.contacts.push_back({index(field(c, "bone")), vec(field(c, "point"))});
    }
  if (o.contains("locomotion")) {
    auto v = object(o["locomotion"]);
    Locomotion l;
    l.groundY = number(field(v, "groundY"));
    auto legs = array(field(v, "legs"), 2);
    require(legs.size() == 2, "locomotion needs two legs");
    for (int i = 0; i < 2; i++) {
      auto a = object(legs[i]);
      l.legs[i] = {index(field(a, "hip")), index(field(a, "knee")),
                   index(field(a, "ankle")), vec(field(a, "contact")),
                   integer(field(a, "bend"))};
    }
    auto optional = [&](const char *key, double &target) {
      if (v.contains(key))
        target = number(v[key]);
    };
    optional("speed", l.speed);
    optional("acceleration", l.acceleration);
    optional("reachThreshold", l.reachThreshold);
    optional("stride", l.stride);
    optional("lift", l.lift);
    optional("minStepDuration", l.minStepDuration);
    optional("maxStepDuration", l.maxStepDuration);
    if (v.contains("secondary")) {
      auto secondary = object(v["secondary"]);
      if (secondary.contains("frequency"))
        l.secondary.frequency = number(secondary["frequency"]);
      if (secondary.contains("damping"))
        l.secondary.damping = number(secondary["damping"]);
      if (secondary.contains("response"))
        l.secondary.response = number(secondary["response"]);
    }
    d.locomotion = l;
  }
  validateDefinition(d);
  QString atlas =
      safeAsset(base, QString::fromStdString(string(field(o, "atlas"))));
  QImageReader reader(atlas);
  reader.setAutoTransform(false);
  auto dimensions = reader.size();
  require(dimensions.isValid() && dimensions.width() <= 2048 &&
              dimensions.height() <= 2048,
          "atlas dimensions exceed 2048 or invalid");
  for (auto a : d.attachments)
    require(qint64(a.source.x) + a.source.width <= dimensions.width() &&
                qint64(a.source.y) + a.source.height <= dimensions.height(),
            "attachment exceeds atlas");
  loaded.atlas = reader.read();
  require(!loaded.atlas.isNull() && loaded.atlas.size() == dimensions,
          "atlas decode failed");
  loaded.atlas =
      loaded.atlas.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  return loaded;
}
} // namespace pets::character
