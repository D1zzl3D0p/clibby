#include "application.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
int main(int argc, char **argv) {
  try {
    QString assets, state;
    bool suppliedCharacter = false, suppliedFixture = false;
    // Parse before GUI initialization: --help and malformed input work even
    // when no display platform can be initialized.
    for (int index = 1; index < argc; ++index) {
      const QString arg = QString::fromLocal8Bit(argv[index]);
      if (arg == "--help" || arg == "-h") {
        std::cout << "Usage: better_pets [--character ABS_DIR | --assets "
                     "ABS_DIR] [--state-dir ABS_DIR]\n"
                     "Resident desktop pet. Use the menu-bar controls or pet "
                     "context menu to quit.\n"
                     "--character  Data-only character directory; defaults to "
                     "the soft creature beside executable.\n"
                     "--assets     Legacy benchmark fixture directory.\n"
                     "--state-dir  Preferences and instance lock; defaults to "
                     "per-user app data.\n";
        return 0;
      }
      if (arg != "--assets" && arg != "--character" && arg != "--state-dir")
        throw std::runtime_error("unknown argument: " + arg.toStdString());
      if (++index >= argc)
        throw std::runtime_error("missing value for " + arg.toStdString());
      const QString value = QString::fromLocal8Bit(argv[index]);
      if (!QDir::isAbsolutePath(value))
        throw std::runtime_error(arg.toStdString() +
                                 " must be an absolute directory");
      if (arg == "--assets" || arg == "--character") {
        assets = value;
        suppliedCharacter |= arg == "--character";
        suppliedFixture |= arg == "--assets";
        if (suppliedCharacter && suppliedFixture)
          throw std::runtime_error("use either --character or --assets");
      } else
        state = value;
    }
    QApplication app(argc, argv);
    app.setApplicationName("Better Pets");
    app.setOrganizationName("Better Pets");
    app.setQuitOnLastWindowClosed(false);
    if (assets.isEmpty())
      assets =
          QCoreApplication::applicationDirPath() + "/characters/soft-creature";
    if (state.isEmpty())
      state = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (state.isEmpty())
      throw std::runtime_error(
          "per-user application data directory is unavailable");
    // The documented offscreen platform is deliberately noninteractive.
    // Normal desktop startup requires usable persistent tray controls.
    pets::ResidentApplication resident(
        assets, state, QGuiApplication::platformName() != "offscreen", {},
        pets::IpcMode::Enabled, nullptr, !suppliedCharacter);
    return app.exec();
  } catch (const pets::AlreadyRunning &error) {
    std::cout << error.what() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}
