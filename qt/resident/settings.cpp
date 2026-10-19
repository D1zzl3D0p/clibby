#include "settings.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cmath>

namespace pets {
namespace {
constexpr qint64 maxSettingsBytes = 16384;
constexpr int maxCoordinate = 1000000;
bool validDisplay(const QString &display) {
  if (display.size() > 256)
    return false;
  for (const auto ch : display)
    if (ch.category() == QChar::Other_Control)
      return false;
  return true;
}
bool validCoordinate(QJsonValue value) {
  if (!value.isDouble())
    return false;
  const double number = value.toDouble();
  return std::isfinite(number) && std::floor(number) == number &&
         number >= -maxCoordinate && number <= maxCoordinate;
}
bool validPreferences(const Preferences &p) {
  const auto validCharacter = [&] {
    if (p.character.isEmpty() || p.character.size() > 64)
      return false;
    for (const auto ch : p.character)
      if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_'))
        return false;
    return true;
  };
  return std::isfinite(p.walkingSpeed) && p.walkingSpeed >= 0.5 && p.walkingSpeed <= 3.0 &&
         std::isfinite(p.companionScale) && p.companionScale >= 0.5 && p.companionScale <= 2.0 &&
         std::isfinite(p.pixelation) && p.pixelation >= 0.25 && p.pixelation <= 4.0 &&
         validDisplay(p.display) && validCharacter() &&
         (!p.hasPosition || (qAbs(qint64(p.position.x())) <= maxCoordinate &&
                             qAbs(qint64(p.position.y())) <= maxCoordinate));
}
} // namespace
Preferences loadPreferences(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || file.size() > maxSettingsBytes)
    return {};
  const auto bytes = file.read(maxSettingsBytes + 1);
  if (bytes.size() > maxSettingsBytes)
    return {};
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(bytes, &error);
  if (error.error != QJsonParseError::NoError || !document.isObject())
    return {};
  const auto object = document.object();
  if (!object["version"].isDouble() || object["version"].toDouble() != 1)
    return {};
  Preferences p;
  if (object.contains("paused")) {
    if (!object["paused"].isBool())
      return {};
    p.paused = object["paused"].toBool();
  }
  if (object.contains("walkingSpeed")) {
    if (!object["walkingSpeed"].isDouble()) return {};
    p.walkingSpeed = object["walkingSpeed"].toDouble();
  }
  if (object.contains("companionScale")) {
    if (!object["companionScale"].isDouble()) return {};
    p.companionScale = object["companionScale"].toDouble();
  }
  if (object.contains("pixelation")) {
    if (!object["pixelation"].isDouble()) return {};
    p.pixelation = object["pixelation"].toDouble();
  }
  if (object.contains("display")) {
    if (!object["display"].isString())
      return {};
    p.display = object["display"].toString();
  }
  if (object.contains("character")) {
    if (!object["character"].isString())
      return {};
    p.character = object["character"].toString();
  }
  if (object.contains("position")) {
    if (!object["position"].isObject())
      return {};
    const auto position = object["position"].toObject();
    if (!validCoordinate(position["x"]) || !validCoordinate(position["y"]))
      return {};
    p.position = {int(position["x"].toDouble()), int(position["y"].toDouble())};
    p.hasPosition = true;
  }
  return validPreferences(p) ? p : Preferences{};
}
bool savePreferences(const QString &path, const Preferences &p) {
  if (!validPreferences(p))
    return false;
  QJsonObject object{{"version", 1},
                     {"paused", p.paused},
                     {"walkingSpeed", p.walkingSpeed},
                     {"companionScale", p.companionScale},
                     {"pixelation", p.pixelation},
                     {"character", p.character},
                     {"display", p.display}};
  if (p.hasPosition)
    object["position"] =
        QJsonObject{{"x", p.position.x()}, {"y", p.position.y()}};
  const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  if (file.write(bytes) != bytes.size()) {
    file.cancelWriting();
    return false;
  }
  return file.commit();
}
} // namespace pets
