#include "application.h"
#include <QAction>
#include <QApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
}
int main(int argc, char **argv) {
  QQuickWindow::setSceneGraphBackend("rhi");
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  QApplication gui(argc, argv);
  gui.setQuitOnLastWindowClosed(false);
  if (argc != 2) return 2;
  try {
    const QString root = QString::fromLocal8Bit(argv[1]);
    QTemporaryDir storage;
    double now = 0;
    pets::ResidentApplication shell(root + "/soft-creature", storage.path(), false,
        [&] { return now; }, pets::IpcMode::DisabledForTests, nullptr, false,
        [] { return std::optional<QPointF>{}; });
    auto tick = [&](double seconds) {
      for (int frame = 0; frame < int(std::ceil(seconds * 120)); ++frame) {
        now += 1. / 120;
        check(QMetaObject::invokeMethod(shell.findChild<QTimer *>(), "timeout", Qt::DirectConnection), "single resident timer callback");
      }
    };
    auto *action = shell.findChild<QAction *>("signature");
    check(action && !action->isEnabled() && !shell.signature(), "planar pet must not advertise a spatial signature");
    QString error;
    auto selectPip = [&] {
      check(shell.selectCharacter(root + "/pip-3d", &error), error.toUtf8().constData());
      tick(.5);
      check(action->isEnabled(), "signature control must follow selected pet");
    };
    selectPip();
    check(shell.signature() && shell.signatureActive() && shell.groundMotionActive(), "signature must wake existing motion scheduler");
    check(!shell.signature(), "signature cannot restart while active");
    tick(.3);
    shell.control(pets::Control::Pause);
    const auto frozen = *shell.groundedState();
    check(!shell.groundMotionActive(), "pause removes signature animation deadline");
    tick(30);
    check(shell.signatureActive(), "pause must freeze action instead of consuming wall time");
    for (std::size_t i = 0; i < frozen.feet.size(); ++i)
      check(frozen.feet[i].position.x == shell.groundedState()->feet[i].position.x &&
            frozen.feet[i].position.y == shell.groundedState()->feet[i].position.y, "pause freezes signature contacts");
    shell.control(pets::Control::Resume);
    check(shell.groundMotionActive(), "resume wakes remaining signature motion");
    tick(7);
    check(!shell.signatureActive(), "finite signature returns to rest after resume");
    bool quiet = !shell.groundMotionActive();
    for (int frame = 0; frame < 480 && !quiet; ++frame) { tick(1. / 120); quiet = !shell.groundMotionActive(); }
    check(quiet, "completed signature must release frame-rate timer deadline");

    selectPip();
    check(shell.signature(), "second signature starts");
    tick(.3);
    shell.setGroundTarget(shell.groundedState()->rootX + 45);
    tick(1);
    check(!shell.signatureActive(), "manual navigation fades signature out");
    selectPip();
    check(shell.signature(), "signature before notification starts");
    tick(.3);
    check(shell.handleRequest("session.update", {{"source_id", "signature-test"}, {"session_id", "reaction"},
        {"label", "Done"}, {"seq", 1}, {"status", "completed"}, {"ttl", 10}}).errorCode == 0,
        "notification reaction accepted");
    check(!shell.signature(), "notification owns behavior priority");
    tick(1);
    check(!shell.signatureActive(), "notification preempts signature with bounded release");
    check(shell.handleRequest("session.clear", {{"source_id", "signature-test"}, {"session_id", "reaction"}, {"seq", 2}}).errorCode == 0,
        "notification clears");
    tick(4);
    selectPip();
    check(shell.signature(), "signature before drag starts");
    shell.control(pets::Control::BeginDrag);
    check(!shell.signatureActive() && !shell.signature(), "drag immediately cancels and suppresses signature");
    shell.control(pets::Control::EndDrag);
    selectPip();
    check(shell.signature(), "signature before hide starts");
    shell.control(pets::Control::Hide);
    tick(100);
    check(!shell.signatureActive() && !shell.signature(), "hidden pet has no live signature");
    shell.control(pets::Control::Show);
    selectPip();
    check(shell.signature(), "signature before selection starts");
    check(shell.selectCharacter(root + "/soft-creature", &error), "switch to planar pet");
    check(!shell.signatureActive() && !action->isEnabled(), "selection discards old signature and control capability");

    selectPip();
    bool automatic = false;
    for (int frame = 0; frame < 120 * 90 && !automatic; ++frame) {
      tick(1. / 120);
      automatic = shell.signatureActive();
    }
    check(automatic, "quiet pet receives a sparse automatic signature opportunity");
    shell.control(pets::Control::Quit);
    std::cout << "resident signature scheduling, pause, cancellation, selection and quiet recovery verified\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
