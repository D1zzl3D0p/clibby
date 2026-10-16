#include "motion.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const QString &message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}
static void compare(const QJsonValue &actual, const QJsonValue &expected, const QString &path) {
    require(actual.type() == expected.type(), path + ": type mismatch");
    if (expected.isDouble()) {
        require(std::isfinite(actual.toDouble()) && std::abs(actual.toDouble()-expected.toDouble()) <= 0.0001,
                path + ": golden mismatch");
    } else if (expected.isObject()) {
        const auto object = expected.toObject();
        for (auto it = object.begin(); it != object.end(); ++it)
            compare(actual.toObject().value(it.key()), it.value(), path + "." + it.key());
    } else if (expected.isArray()) {
        require(actual.toArray().size() == expected.toArray().size(), path + ": array length");
        for (int i = 0; i < expected.toArray().size(); ++i)
            compare(actual.toArray()[i], expected.toArray()[i], path + QString::number(i));
    } else require(actual == expected, path + ": value mismatch");
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        require(argc == 2, "pass assets directory");
        const QString assets = QString::fromLocal8Bit(argv[1]);
        const Rig rig = loadRig(assets);
        const auto entries = readJson(assets + "/golden.json")["poses"].toArray();
        require(!entries.empty(), "golden poses must not be empty");
        for (const auto &entry : entries) {
            auto expected = entry.toObject();
            const auto tick = expected.take("tick").toInteger();
            const auto scenario = expected.take("scenario").toString();
            compare(evaluatePose(rig, tick, scenario).json(rig), expected, scenario + "/" + QString::number(tick));
        }
        require(phaseAt(44.999, "mixed-active") == "idle-motion", "before 45s");
        require(phaseAt(45, "mixed-active") == "walk-desktop", "at 45s");
        require(phaseAt(60, "mixed-active") == "idle-motion", "at 60s");
        require(phaseAt(40, "mixed-rest") == "walk-desktop", "rest 40s");
        require(phaseAt(55, "mixed-rest") == "idle-motion", "rest 55s");
        require(evaluatePose(rig, 0, "static").json(rig) == evaluatePose(rig, 600, "static").json(rig), "static must freeze");
        require(evaluatePose(rig, 2700, "mixed-active").root.x() == 0, "first walk is tick 2701");
        require(evaluatePose(rig, 2701, "mixed-active").root.x() > 0, "walk begins at 2701");
        Motion state(rig);
        state.step("walk-desktop");
        const auto frozen = state.pose().json(rig);
        for (int i = 0; i < 100; ++i) state.step("hidden");
        require(state.pose().json(rig) == frozen, "hidden preserves all transforms");
        state.step("walk-desktop");
        compare(state.pose().json(rig), evaluatePose(rig, 2, "walk-desktop").json(rig), "resume no replay");
        bool rejected = false;
        try { evaluatePose(rig, 10, "typo"); } catch (const std::exception &) { rejected = true; }
        require(rejected, "reject unknown scenario");
        rejected = false;
        try { evaluatePose(rig, -1, "static"); } catch (const std::exception &) { rejected = true; }
        require(rejected, "reject negative tick");
        std::cout << "PASS " << entries.size() << " golden poses, boundaries, frozen/resume, invalid input\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
