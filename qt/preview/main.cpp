#include "behavior/curiosity.h"
#include "characters/item.h"
#include "characters/grounded.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QQuickWindow>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
using namespace pets::character;
class GroundOverlay final : public QQuickPaintedItem {
public:
  explicit GroundOverlay(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setZ(10); setAcceptedMouseButtons(Qt::NoButton);
  }
  QPointF anchor; double scale=1, floor=0, target=0;
  GroundedState state; bool debug=false;
  void paint(QPainter *p) override {
    p->setPen(QColor(105,110,125)); p->drawLine(QPointF(0,floor),QPointF(width(),floor));
    const double x=anchor.x()+(target-state.rootX)*scale;
    p->setPen(QColor(230,181,85));
    p->drawLine(QPointF(x,floor-18),QPointF(x,floor+3));
    if(debug) for(const auto &foot:state.feet) {
      p->setPen(foot.swing?QColor(244,151,104):QColor(107,210,160));
      p->drawEllipse(QPointF(anchor.x()+(foot.position.x-state.rootX)*scale,
                            floor+foot.position.y*scale),3,3);
      p->drawLine(QPointF(anchor.x()+(foot.landing.x-state.rootX)*scale,floor-4),
                  QPointF(anchor.x()+(foot.landing.x-state.rootX)*scale,floor+4));
    }
  }
};
class PreviewWindow : public QQuickWindow {
public:
  std::function<void(QPointF, bool)> input;
  std::function<void()> resize;

protected:
  void mouseMoveEvent(QMouseEvent *e) override {
    if (input)
      input(e->position(), false);
    QQuickWindow::mouseMoveEvent(e);
  }
  void mousePressEvent(QMouseEvent *e) override {
    if (input)
      input(e->position(), true);
    QQuickWindow::mousePressEvent(e);
  }
  void resizeEvent(QResizeEvent *e) override {
    QQuickWindow::resizeEvent(e);
    if (resize)
      resize();
  }
};
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions(
      {{"autonomous", "Enable calm curious activities"},
       {"seed", "Reproducible autonomous behavior seed", "integer", "1"},
       {"character", "Character directory", "directory"},
       {"capture", "Capture PNG and JSON sidecar", "output"},
       {"action", "rest|idle|walk|attend|celebrate|failure|acknowledge", "name",
        "idle"},
       {"time", "Controlled capture time", "seconds", "0"},
       {"mode", "direct|composited", "mode", "direct"},
       {"resolution", "64|96|128", "pixels", "96"},
       {"dither", "Ordered palette dither"},
       {"palette", "Reduce output palette"},
       {"linear", "Linear attachment sampling"},
       {"debug", "Show bone/contact overlay (CPU composition)"},
       {"impulse", "Direction applied at time zero", "X,Y"},
       {"impulse-strength", "Bounded impulse magnitude (-4..4)", "strength",
        "1"},
       {"gaze", "Controlled character-local gaze offset", "X,Y", "0,0"},
       {"target", "Grounded target X relative to starting position (-96..96)", "X"},
       {"hop", "Procedural hop height", "height"},
       {"fly", "Powered flight target X,elevation", "X,Y"},
       {"land-at", "Begin landing at simulation seconds", "seconds"},
       {"speed", "Travel speed setting (0.5..3; limited by leg reach)", "multiplier", "2"},
       {"reverse-at", "Reverse target after elapsed seconds", "seconds"},
       {"duration", "Bounded live run duration", "seconds"}});
  parser.process(app);
  if (parser.isSet("capture") && parser.value("capture").isEmpty()) {
    std::cerr << "Capture output required\n";
    return 2;
  }
  const QStringList names = {"rest",      "idle",    "walk",       "attend",
                             "celebrate", "failure", "acknowledge"};
  int actionIndex = names.indexOf(parser.value("action"));
  bool timeOk = false;
  double time = parser.value("time").toDouble(&timeOk);
  bool resolutionOk = false;
  int resolution = parser.value("resolution").toInt(&resolutionOk);
  if (actionIndex < 0 || !timeOk || !std::isfinite(time) || time < 0 ||
      !resolutionOk ||
      (resolution != 64 && resolution != 96 && resolution != 128) ||
      (parser.value("mode") != "direct" &&
       parser.value("mode") != "composited")) {
    std::cerr << "Invalid preview option\n";
    return 2;
  }
  bool seedOk = false;
  const auto seed = parser.value("seed").toUInt(&seedOk);
  if (!seedOk) { std::cerr << "Invalid behavior seed\n"; return 2; }
  double duration = 0;
  if (parser.isSet("duration")) {
    bool ok;
    duration = parser.value("duration").toDouble(&ok);
    if (!ok || !std::isfinite(duration) || duration <= 0 || duration > 86400) {
      std::cerr << "Invalid duration\n";
      return 2;
    }
  }
  auto vectorOption = [&](const QString &value, Vec2 &result) {
    const auto fields = value.split(',');
    if (fields.size() != 2)
      return false;
    bool xOk = false, yOk = false;
    result = {fields[0].toDouble(&xOk), fields[1].toDouble(&yOk)};
    return xOk && yOk && std::isfinite(result.x) && std::isfinite(result.y) &&
           std::abs(result.x) <= 8192 && std::abs(result.y) <= 8192;
  };
  Vec2 initialGaze, initialImpulse;
  bool strengthOk = false;
  double strength = parser.value("impulse-strength").toDouble(&strengthOk);
  if (!vectorOption(parser.value("gaze"), initialGaze) ||
      (parser.isSet("impulse") &&
       !vectorOption(parser.value("impulse"), initialImpulse)) ||
      !strengthOk || !std::isfinite(strength) || std::abs(strength) > 4) {
    std::cerr << "Invalid gaze/impulse option\n";
    return 2;
  }
  QString directory = parser.value("character");
  if (directory.isEmpty()) {
    directory = QDir(QCoreApplication::applicationDirPath())
                    .filePath("characters/soft-creature");
    if (!QDir(directory).exists())
      directory = QDir::current().filePath("characters/soft-creature");
  }
  LoadedCharacter loaded;
  try {
    loaded = loadCharacter(directory);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
  double targetX=0, reverseAt=-1;
  bool speedValid=false;
  const double walkingSpeed=parser.value("speed").toDouble(&speedValid);
  if(!speedValid || !std::isfinite(walkingSpeed) || walkingSpeed<.5 || walkingSpeed>3) {
    std::cerr<<"Invalid travel speed setting\n"; return 2;
  }
  for (const auto &key: {QString("target"),QString("reverse-at")}) if(parser.isSet(key)) {
    bool ok=false; const double value=parser.value(key).toDouble(&ok);
    if(!ok || !std::isfinite(value) || !loaded.definition.locomotion ||
       (key=="target"? std::abs(value)>96 : value<0 || value>30) || time>30) {
      std::cerr<<"Invalid grounded option\n"; return 2;
    }
    if(key=="target") targetX=value; else reverseAt=value;
  }
  if(loaded.definition.locomotion && !parser.isSet("target") && actionIndex==2) targetX=60;
  QWidget panel;
  panel.setWindowTitle("Better Pets character preview");
  auto *layout = new QVBoxLayout(&panel);
  auto *controls = new QHBoxLayout;
  layout->addLayout(controls);
  auto *load = new QPushButton("Load character");
  controls->addWidget(load);
  auto *actions = new QComboBox;
  actions->addItems(names);
  actions->setCurrentIndex(actionIndex);
  controls->addWidget(actions);
  auto *modes = new QComboBox;
  modes->addItems({"Direct retained", "CPU composited (experimental)"});
  modes->setCurrentIndex(parser.value("mode") == "composited" ||
                        (!parser.isSet("mode") && loaded.definition.locomotion));
  controls->addWidget(modes);
  auto *resolutions = new QComboBox;
  resolutions->addItems({"64", "96", "128"});
  resolutions->setCurrentText(QString::number(resolution));
  controls->addWidget(resolutions);
  auto *second = new QHBoxLayout;
  layout->addLayout(second);
  auto *palette = new QCheckBox("Palette");
  palette->setChecked(parser.isSet("palette"));
  auto *dither = new QCheckBox("Dither");
  dither->setChecked(parser.isSet("dither"));
  auto *smooth = new QCheckBox("Linear piece filter");
  smooth->setChecked(parser.isSet("linear"));
  auto *debug = new QCheckBox("Bones / contacts");
  debug->setChecked(parser.isSet("debug"));
  auto *autonomous = new QCheckBox("Calm curiosity");
  autonomous->setChecked(parser.isSet("autonomous"));
  auto *pause = new QCheckBox("Pause");
  auto *reset = new QPushButton("Reset");
  for (auto *box : {palette, dither, smooth, debug, autonomous, pause})
    second->addWidget(box);
  second->addWidget(reset);
  auto *speed = new QDoubleSpinBox;
  speed->setObjectName("walkingSpeed");
  speed->setRange(.5, 3);
  speed->setSingleStep(.25);
  speed->setSuffix("×");
  speed->setValue(walkingSpeed);
  speed->setAccessibleName("Travel speed multiplier");
  speed->setToolTip("Moves the body at the selected speed. Feet follow and catch up when needed.");
  second->addWidget(new QLabel("Travel speed"));
  second->addWidget(speed);
  auto *groundControls=new QHBoxLayout;
  layout->addLayout(groundControls);
  auto *targetControl=new QDoubleSpinBox; targetControl->setRange(-96,96); targetControl->setValue(targetX);
  groundControls->addWidget(new QLabel("Ground target X")); groundControls->addWidget(targetControl);
  auto *frequency=new QDoubleSpinBox; frequency->setRange(.2,12); frequency->setValue(3);
  auto *damping=new QDoubleSpinBox; damping->setRange(.2,3); damping->setSingleStep(.1); damping->setValue(.8);
  auto *response=new QDoubleSpinBox; response->setRange(-3,3); response->setSingleStep(.1);
  for (auto pair: {std::pair<const char*,QDoubleSpinBox*>{"Frequency",frequency},{"Damping",damping},{"Response",response}}) {
    groundControls->addWidget(new QLabel(pair.first)); groundControls->addWidget(pair.second);
  }
  auto *info = new QLabel;
  info->setWordWrap(true);
  layout->addWidget(info);
  auto *view = new PreviewWindow;
  view->setColor(QColor(36, 39, 48));
  auto *container = QWidget::createWindowContainer(view);
  container->setMinimumSize(600, 460);
  layout->addWidget(container, 1);
  auto *status = new QLabel;
  layout->addWidget(status);
  std::unique_ptr<Controller> controller =
      std::make_unique<Controller>(loaded.definition);
  CharacterItem *item = new CharacterItem(loaded, view->contentItem());
  CharacterItem *actual = new CharacterItem(loaded, view->contentItem());
  auto *overlay=new GroundOverlay(view->contentItem());
  std::unique_ptr<GroundedController> grounded;
  auto resetGround=[&] {
    grounded.reset();
    if(loaded.definition.locomotion) {
      grounded=std::make_unique<GroundedController>(loaded.definition);
      grounded->setSpeedMultiplier(speed->value());
      grounded->reset(0,{-96,96}); grounded->setTargetX(targetX);
      const auto tuning=loaded.definition.locomotion->secondary;
      const QSignalBlocker blockFrequency(frequency), blockDamping(damping), blockResponse(response);
      frequency->setValue(tuning.frequency);
      damping->setValue(tuning.damping);
      response->setValue(tuning.response);
    }
    overlay->setVisible(bool(grounded));
    targetControl->setEnabled(bool(grounded));
    speed->setEnabled(bool(grounded));
  };
  resetGround();
  pets::behavior::Curiosity curiosity(seed);
  double behaviorTime = 0, manualUntil = parser.isSet("target") ? 1 : 0;
  auto expression = [&]() -> ExpressionMotion & { return grounded ? grounded->expressions() : controller->expressions(); };
  auto behaviorCapabilities = [&] {
    pets::behavior::Capabilities capabilities;
    for (unsigned index=0; index<unsigned(Role::Count); ++index) {
      auto role=static_cast<Role>(index);
      if (expression().supports(role)) capabilities.roles |= roleBit(role);
    }
    capabilities.navigation=bool(grounded);
    return capabilities;
  };
  auto resetCuriosity = [&] {
    expression().reset();
    if (grounded) grounded->setAction(Action::Rest, 0); else controller->setAction(Action::Rest, 0);
    const auto capabilities=behaviorCapabilities();
    auto profile=loaded.definition.id=="soft-creature" ? pets::behavior::Profile::Mallow : pets::behavior::Profile::Bramble;
    curiosity.reset(behaviorTime, profile, capabilities);
  };
  resetCuriosity();
  double landAt=-1;
  if(parser.isSet("land-at")) {
    bool ok=false; landAt=parser.value("land-at").toDouble(&ok);
    if(!ok || !std::isfinite(landAt) || landAt<0 || landAt>180) return 2;
  }
  if(parser.isSet("hop")) {
    bool ok=false; const double height=parser.value("hop").toDouble(&ok);
    if(!grounded || !ok || !std::isfinite(height) || height<=0 || height>160 || !grounded->hop(height)) return 2;
  }
  if(parser.isSet("fly")) {
    Vec2 goal;
    if(!grounded || !vectorOption(parser.value("fly"),goal) || std::abs(goal.x)>96 || goal.y<0 || goal.y>240 || !grounded->flyTo(goal.x,goal.y)) return 2;
  }
  bool landing=false;
  auto activityStep = [&](double elapsed) {
    pets::behavior::Observation observation;
    observation.width=loaded.definition.width;
    observation.suppressed=!autonomous->isChecked() || pause->isChecked();
    if (grounded) {
      const auto state=grounded->state(); observation.rootX=state.rootX;
      observation.suppressed=observation.suppressed || state.phase!=DropPhase::Grounded;
      observation.spanMin=-96; observation.spanMax=96;
      observation.arrived=std::abs(state.rootX-state.targetX)<.05 && std::abs(state.velocity)<.02
          && !state.feet[0].swing && !state.feet[1].swing;
      observation.manualBusy=elapsed<manualUntil || (manualUntil>0 && !observation.arrived);
    }
    for (const auto &command : curiosity.advance(elapsed, observation)) {
      using Kind=pets::behavior::Command::Kind;
      switch(command.kind) {
      case Kind::Look: expression().look(command.value); break;
      case Kind::RoleGoal: expression().goal(command.role,command.value); break;
      case Kind::RoleImpulse: expression().impulse(command.role,command.value); break;
      case Kind::Clear: expression().clear(command.role); break;
      case Kind::Navigate: if(grounded) grounded->setTargetX(command.targetX); break;
      case Kind::StopNavigation: if(grounded) grounded->setTargetX(grounded->state().rootX); break;
      case Kind::Reach: case Kind::ReleaseReach: break;
      }
    }
    if (observation.arrived && elapsed>=manualUntil) manualUntil=0;
  };
  QObject::connect(speed,qOverload<double>(&QDoubleSpinBox::valueChanged),&panel,
                   [&](double value) { if(grounded) grounded->setSpeedMultiplier(value); });
  Vec2 gaze = initialGaze;
  bool capture = !parser.value("capture").isEmpty();
  if (parser.isSet("impulse")) {
    if (grounded) grounded->impulse(initialImpulse, strength);
    else controller->impulse(initialImpulse, strength);
  }
  auto submitManual = [&](double elapsed) {
    if (autonomous->isChecked()) return;
    if (grounded) grounded->setAction(static_cast<Action>(actions->currentIndex()), elapsed);
    else controller->setAction(static_cast<Action>(actions->currentIndex()), elapsed);
    if (gaze.x != 0 || gaze.y != 0)
      expression().look({gaze.x/180.0, -gaze.y/180.0, 1});
  };
  auto stepGround=[&](double dt) {
    if(!grounded) return;
    if(landAt>=0 && behaviorTime>=landAt && !landing) { grounded->land(); landing=true; }
    const auto before=grounded->state();
    auto request=grounded->propose(dt);
    grounded->realize(before.rootX+request.displacement,false,std::max(0.,before.elevation-request.displacementY));
  };
  bool reversed=false;
  if(capture) for(double elapsed=0;elapsed<std::min(time,180.0);) {
    if(reverseAt>=0 && elapsed>=reverseAt && !reversed && grounded) {
      grounded->setTargetX(-targetX); reversed=true;
    }
    const double dt=std::min(1.0/120,std::min(time,180.0)-elapsed);
    activityStep(elapsed); submitManual(elapsed);
    if (grounded) stepGround(dt); else controller->step(dt);
    elapsed+=dt; behaviorTime=elapsed;
  }
  status->setText(QString::fromStdString(loaded.definition.name) + " — " +
                  directory);
  auto configure = [&] {
    for (auto *i : {item, actual}) {
      i->setTransformOrigin(QQuickItem::TopLeft);
      i->setMode(modes->currentIndex() ? RenderMode::Composited
                                       : RenderMode::Direct);
      i->setResolution(resolutions->currentText().toInt());
      i->setPalette(palette->isChecked());
      i->setDither(dither->isChecked());
      i->setSmooth(smooth->isChecked());
      i->setDebugOverlay(debug->isChecked());
      i->setPose(grounded ? grounded->pose(Action::Rest, 0)
                          : controller->pose(Action::Rest, 0));
    }
  };
  auto arrange = [&] {
    const double dpr = view->devicePixelRatio();
    const double fit = std::min((view->width() - 40) / (item->width()+(grounded?192:0)),
                                (view->height() - 130) / item->height());
    const bool cpu = modes->currentIndex() || palette->isChecked() ||
                     dither->isChecked() || debug->isChecked();
    const double outputUnit = cpu ? std::max(item->width(), item->height()) /
                                        resolutions->currentText().toDouble()
                                  : 1;
    const double physical = std::max(1.0, std::floor(fit * dpr * outputUnit));
    item->setScale(physical / (dpr * outputUnit));
    item->setPosition(QPointF(
        (view->width() - item->width() * item->scale()) / 2 +
            (grounded?grounded->state().rootX*item->scale():0),
        130 + (view->height() - 130 - item->height() * item->scale()) / 2
            -(grounded?grounded->state().elevation*item->scale():0)));
    actual->setScale(1 / (dpr * outputUnit));
    actual->setPosition(QPointF(15, 15));
    if(grounded) {
      overlay->setSize(view->size()); overlay->state=grounded->state();
      overlay->scale=item->scale(); overlay->target=grounded->state().targetX;
      overlay->anchor=item->position()+QPointF((loaded.definition.origin.x+loaded.definition.motionEnvelope)*item->scale(),0);
      overlay->floor=item->y()+(loaded.definition.locomotion->groundY+loaded.definition.motionEnvelope+grounded->state().elevation)*item->scale();
      overlay->debug=debug->isChecked(); overlay->update();
    }
    info->setText(
        QString("Small view: 1 physical pixel per output pixel. Large: %1 "
                "physical pixels/output pixel; DPR %2. Canvas %3. Nearest "
                "final enlargement; no antialiasing. Palette/dither/debug opt "
                "into CPU composition. Native scaling remains unverified.")
            .arg(physical)
            .arg(dpr)
            .arg(resolutions->currentText()));
  };
  view->resize = arrange;
  auto change = [&] {
    configure();
    arrange();
  };
  QObject::connect(actions, &QComboBox::currentIndexChanged, &panel, [&] {
    autonomous->setChecked(false); submitManual(time); change();
  });
  QObject::connect(autonomous, &QCheckBox::toggled, &panel, [&](bool) {
    activityStep(behaviorTime);
    expression().clear();
    curiosity.reset(behaviorTime, loaded.definition.id=="soft-creature" ? pets::behavior::Profile::Mallow : pets::behavior::Profile::Bramble,
        behaviorCapabilities());
  });
  auto *primitiveControls = new QHBoxLayout;
  layout->insertLayout(2, primitiveControls);
  for (auto entry : {std::pair<const char *,Role>{"Head tilt",Role::Head},
                     {"Left ear twitch",Role::EarLeft}, {"Right ear twitch",Role::EarRight},
                     {"Tail swish",Role::Tail}}) {
    auto *button = new QPushButton(entry.first);
    primitiveControls->addWidget(button);
    const auto role = entry.second;
    QObject::connect(button,&QPushButton::clicked,&panel,[&,role] {
      autonomous->setChecked(false);
      if (role==Role::EarLeft || role==Role::EarRight) expression().impulse(role,{0,0,role==Role::EarLeft?1.4:-1.4});
      else expression().goal(role,{0,0,role==Role::Head?.2:.35});
    });
  }
  auto *hopButton=new QPushButton("Hop");
  auto *flyButton=new QPushButton("Fly");
  auto *landButton=new QPushButton("Land");
  primitiveControls->addWidget(hopButton); primitiveControls->addWidget(flyButton); primitiveControls->addWidget(landButton);
  QObject::connect(hopButton,&QPushButton::clicked,&panel,[&] {
    if(grounded && !pause->isChecked()) {autonomous->setChecked(false);grounded->hop(30);}
  });
  QObject::connect(flyButton,&QPushButton::clicked,&panel,[&] {
    if(grounded && !pause->isChecked()) {autonomous->setChecked(false);grounded->flyTo(targetControl->value(),55);}
  });
  QObject::connect(landButton,&QPushButton::clicked,&panel,[&] {
    if(grounded && !pause->isChecked()) {autonomous->setChecked(false);grounded->land();}
  });
  hopButton->setEnabled(bool(grounded)); flyButton->setEnabled(grounded && grounded->flightCapable()); landButton->setEnabled(bool(grounded));
  auto *releaseGoals = new QPushButton("Release goals");
  primitiveControls->addWidget(releaseGoals);
  QObject::connect(releaseGoals,&QPushButton::clicked,&panel,[&] { autonomous->setChecked(false); expression().clear(); });
  QObject::connect(targetControl,qOverload<double>(&QDoubleSpinBox::valueChanged),&panel,[&](double x) {
    targetX=x; manualUntil=behaviorTime+.25; activityStep(behaviorTime);
    if(grounded)grounded->setTargetX(x); change();
  });
  auto tuning=[&] { if(grounded)grounded->setSecondaryTuning({frequency->value(),damping->value(),response->value()}); };
  for(auto *spin:{frequency,damping,response})
    QObject::connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),&panel,tuning);
  QObject::connect(modes, &QComboBox::currentIndexChanged, &panel, change);
  QObject::connect(resolutions, &QComboBox::currentIndexChanged, &panel,
                   change);
  for (auto *box : {palette, dither, smooth, debug})
    QObject::connect(box, &QCheckBox::toggled, &panel, change);
  QObject::connect(pause, &QCheckBox::toggled, &panel, [&](bool) {
    activityStep(behaviorTime);
  });
  QObject::connect(reset, &QPushButton::clicked, &panel, [&] {
    time = 0;
    gaze = {};
    controller->resetDynamics();
    targetX=0; targetControl->setValue(0); resetGround();
    behaviorTime=0; resetCuriosity();
    change();
  });
  QObject::connect(load, &QPushButton::clicked, &panel, [&] {
    QString selected = QFileDialog::getExistingDirectory(
        &panel, "Select data-only character", directory);
    if (selected.isEmpty())
      return;
    try {
      auto candidate = loadCharacter(selected);
      auto replacement = std::make_unique<Controller>(candidate.definition);
      delete item;
      delete actual;
      loaded = std::move(candidate);
      controller = std::move(replacement);
      resetGround();
      resetCuriosity();
      if(!parser.isSet("mode")) modes->setCurrentIndex(bool(grounded));
      directory = selected;
      item = new CharacterItem(loaded, view->contentItem());
      actual = new CharacterItem(loaded, view->contentItem());
      time = 0;
      gaze = {};
      change();
      status->setText(QString::fromStdString(loaded.definition.name) + " — " +
                      directory);
    } catch (const std::exception &e) {
      status->setText("Kept current character: " + QString::fromUtf8(e.what()));
    }
  });
  view->input = [&](QPointF point, bool click) {
    if (pause->isChecked())
      return;
    auto local = item->mapFromScene(point);
    if (!std::isfinite(local.x()) || !std::isfinite(local.y()))
      return;
    autonomous->setChecked(false);
    gaze = {std::clamp(local.x() - item->width() / 2, -8192.0, 8192.0),
            std::clamp(local.y() - item->height() / 2, -8192.0, 8192.0)};
    if (click || (grounded && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))) {
      if(grounded && !(QGuiApplication::keyboardModifiers()&Qt::ControlModifier))
        targetControl->setValue(grounded->state().rootX+local.x()-loaded.definition.origin.x-loaded.definition.motionEnvelope);
      else if(grounded)grounded->impulse(gaze,1);
      else controller->impulse(gaze,1);
    }
    submitManual(time);
    configure();
    if(grounded)arrange();
  };
  QTimer timer;
  timer.setInterval(16);
  QObject::connect(&timer, &QTimer::timeout, &panel, [&] {
    if (pause->isChecked())
      return;
    double dt = .016;
    time += dt; behaviorTime += dt;
    activityStep(behaviorTime); submitManual(time);
    if (!grounded) controller->step(dt);
    if(reverseAt>=0 && time>=reverseAt && !reversed && grounded) {
      grounded->setTargetX(-targetX); reversed=true;
    }
    stepGround(dt);
    configure();
    if(grounded)arrange();
  });
  if (!capture)
    timer.start();
  panel.resize(960, 800);
  panel.show();
  change();
  if (capture)
    QTimer::singleShot(300, &panel, [&] {
      const QString output = parser.value("capture");
      QImage image = view->grabWindow();
      if (image.isNull() || !image.save(output)) {
        std::cerr << "Capture failed\n";
        app.exit(3);
        return;
      }
      QJsonObject config{
          {"autonomous", autonomous->isChecked()},
          {"seed", double(seed)},
          {"activity", int(curiosity.activity())},
          {"character", QDir(directory).absolutePath()},
          {"character_id", QString::fromStdString(loaded.definition.id)},
          {"character_name", QString::fromStdString(loaded.definition.name)},
          {"debug", debug->isChecked()},
          {"impulse", QJsonArray{initialImpulse.x, initialImpulse.y}},
          {"impulse_strength", parser.isSet("impulse") ? strength : 0.0},
          {"gaze", QJsonArray{gaze.x, gaze.y}},
          {"action", names[actions->currentIndex()]},
          {"time", time},
          {"mode", modes->currentIndex() ? "composited" : "direct"},
          {"effective_cpu_composition",
           bool(modes->currentIndex() || palette->isChecked() ||
                dither->isChecked() || debug->isChecked())},
          {"resolution", resolutions->currentText().toInt()},
          {"dither", dither->isChecked()},
          {"palette", palette->isChecked()},
          {"piece_filter", smooth->isChecked() ? "linear" : "nearest"},
          {"final_filter", "nearest"},
          {"antialiasing", false},
          {"device_pixel_ratio", view->devicePixelRatio()},
          {"physical_scale", item->scale() * view->devicePixelRatio()},
          {"output_pixel_scale",
           item->scale() * view->devicePixelRatio() *
               (modes->currentIndex() || palette->isChecked() ||
                        dither->isChecked() || debug->isChecked()
                    ? std::max(item->width(), item->height()) /
                          resolutions->currentText().toDouble()
                    : 1)},
          {"capture_width", image.width()},
          {"capture_height", image.height()},
          {"platform", QGuiApplication::platformName()},
          {"native_scaling_verified", false}};
      config.insert("grounded",bool(grounded));
      config.insert("walking_speed",speed->value());
      if(grounded) {
        const auto s=grounded->state();
        config.insert("root_x",s.rootX); config.insert("target_x",s.targetX);
        config.insert("elevation",s.elevation); config.insert("phase",int(s.phase)); config.insert("powered_flight",s.poweredFlight);
        QJsonArray wings; for(const auto &w:s.wings) wings.append(QJsonObject{{"shoulder",w.shoulder},{"elbow",w.elbow},{"wrist",w.wrist},{"extension",w.extension},{"stroke_velocity",w.strokeVelocity}});
        config.insert("wings",wings);
        config.insert("motion_active",s.active);
        config.insert("facing",s.facing);
        config.insert("facing_blend",s.facingBlend);
        QJsonArray feet;
        for(const auto &f:s.feet)feet.append(QJsonObject{{"x",f.position.x},{"y",f.position.y},{"swing",f.swing}});
        config.insert("feet",feet);
      }
      QFile file(output + ".json");
      if (!file.open(QIODevice::WriteOnly) ||
          file.write(QJsonDocument(config).toJson()) < 0) {
        app.exit(3);
        return;
      }
      app.quit();
    });
  if (parser.isSet("duration") && !capture)
    QTimer::singleShot(int(duration * 1000), &app, &QApplication::quit);
  return app.exec();
}
