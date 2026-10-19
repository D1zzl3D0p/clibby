#include "item.h"
#include "raster.h"
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickWindow>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace pets::character;
void check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  try {
    Definition d;
    d.width = d.height = 64;
    d.motionEnvelope = 0;
    d.bones = {{"child", 1, {8, 0}}, {"parent", -1, {10, 10}}};
    d.attachments = {{0, {0, 0, 8, 8}, {}, {}, 10},
                     {1, {8, 0, 8, 8}, {}, {}, 20}};
    QImage atlas(16, 8, QImage::Format_ARGB32);
    atlas.fill(Qt::red);
    for (int y = 0; y < 8; ++y)
      for (int x = 8; x < 16; ++x)
        atlas.setPixelColor(x, y, Qt::blue);
    Pose pose;
    pose.bones = {d.bones[0].rest, d.bones[1].rest};
    RasterOptions options;
    options.resolution = 64;
    auto image = renderCharacter(d, atlas, pose, options);
    check(image.pixelColor(0, 0).alpha() == 0, "transparent canvas");
    for (int resolution : {24, 48, 192, 384, 512}) {
      auto adjusted = options; adjusted.resolution = resolution;
      const auto frame = renderCharacter(d, atlas, pose, adjusted);
      check(frame.size() == QSize(resolution,resolution), "bounded adjustable pixel density");
      check(frame.pixelColor(0,0).alpha()==0, "pixel density preserves transparency");
    }
    check(image.pixelColor(19, 11) == QColor(Qt::red),
          "parent-after-child transforms");
    d.bones[0].rest.x = 0;
    pose.bones[0].x = 0;
    image = renderCharacter(d, atlas, pose, options);
    check(image.pixelColor(11, 11) == QColor(Qt::blue),
          "independent attachment ordering");
    pose.bones[1].x = 30;
    auto moved = renderCharacter(d, atlas, pose, options);
    check(moved.pixelColor(11, 11).alpha() == 0, "changed pose moves art");
    Definition blink;
    blink.width=blink.height=64; blink.motionEnvelope=0;
    blink.bones={{"head",-1,{10,10}},{"eyes",0,{10,10}},
                 {"pupil",1,{2,0}},{"foot",-1,{40,40}}};
    blink.bindings["eyes"]=1;
    blink.attachments={{0,{0,0,8,8},{},{},0},
                       {1,{8,0,8,8},{4,4},{},1},
                       {2,{0,0,2,2},{1,1},{},2}};
    blink.contacts={{3,{0,0}}};
    Pose openEyes;
    for(const auto &bone:blink.bones)openEyes.bones.push_back(bone.rest);
    auto closedEyes=openEyes; closedEyes.eyeOpenness=0;
    auto openImage=renderCharacter(blink,atlas,openEyes,options);
    auto closedImage=renderCharacter(blink,atlas,closedEyes,options);
    check(openImage!=closedImage,"blink changes eye artwork");
    check(openImage.pixelColor(11,11)==closedImage.pixelColor(11,11),
          "blink changes head artwork");
    const auto openWorld=worldTransforms(blink,openEyes);
    const auto closedWorld=worldTransforms(blink,closedEyes);
    for(size_t i=0;i<openWorld.size();++i)
      check(openWorld[i].m00==closedWorld[i].m00 &&
            openWorld[i].m01==closedWorld[i].m01 &&
            openWorld[i].m10==closedWorld[i].m10 &&
            openWorld[i].m11==closedWorld[i].m11 &&
            openWorld[i].tx==closedWorld[i].tx &&
            openWorld[i].ty==closedWorld[i].ty,
            "blink changes joint or contact transforms");
    const auto closedArt=artworkTransforms(blink,closedEyes);
    check(std::abs(closedArt[1].m11-.05)<1e-12 &&
          std::abs(closedArt[2].m11-.05)<1e-12,
          "blink must flatten eyes and their artwork descendants");
    blink.bindings.clear();
    check(renderCharacter(blink,atlas,closedEyes,options)==openImage,
          "unsupported eyes binding changes artwork");
    blink.bindings["eyes"]=1;
    auto invalidEyes=openEyes;
    invalidEyes.eyeOpenness=std::numeric_limits<double>::quiet_NaN();
    check(renderCharacter(blink,atlas,invalidEyes,options)==openImage,
          "invalid eye openness must fall back to open eyes");
    invalidEyes.eyeOpenness=2;
    check(renderCharacter(blink,atlas,invalidEyes,options)==openImage,
          "eye openness must clamp to its upper bound");
    invalidEyes.eyeOpenness=-1;
    check(renderCharacter(blink,atlas,invalidEyes,options)==closedImage,
          "eye openness must clamp to its lower bound");
    QImage gradient(16, 16, QImage::Format_ARGB32);
    gradient.fill(QColor(100, 120, 140, 77));
    gradient.setPixelColor(0, 0, QColor(255, 100, 40, 0));
    auto other = gradient;
    reducePalette(gradient, true);
    reducePalette(other, true);
    check(gradient == other, "fixed dither stability");
    check(gradient.pixelColor(1, 1).alpha() == 77 &&
              gradient.pixelColor(0, 0).alpha() == 0,
          "palette preserves alpha");
    check(gradient.pixelColor(1, 1).red() != 100, "palette actually quantizes");
    Definition stretched;
    stretched.width=stretched.height=64; stretched.motionEnvelope=0;
    stretched.bones={{"hip",-1,{10,10}},{"knee",0,{10,0}},
                     {"ankle",1,{10,0}}};
    stretched.attachments={{0,{0,0,10,4},{},{},0}};
    Locomotion gait;
    gait.legs={LegChain{0,1,2,{},1},LegChain{0,1,2,{},1}};
    stretched.locomotion=gait;
    Pose longLeg;
    for(const auto& bone:stretched.bones)longLeg.bones.push_back(bone.rest);
    longLeg.bones[1].x=20;
    QImage limbAtlas(10,4,QImage::Format_ARGB32);limbAtlas.fill(Qt::red);
    const auto extendedArt=renderCharacter(stretched,limbAtlas,longLeg,options);
    check(extendedArt.pixelColor(28,11)==QColor(Qt::red),
          "extended joint leaves a gap in limb artwork");
    check(extendedArt.pixelColor(15,15).alpha()==0,
          "limb extension unnecessarily widens artwork");
    CharacterItem stretchedItem(stretched,limbAtlas);
    stretchedItem.setPose(longLeg);
    check(stretchedItem.visualRegion().contains(QPoint(28,11)),
          "native alpha mask omits extended limb artwork");
    d.bindings["facing"] = 1;
    pose.bones[1].scaleX = -1;
    pose.bones[1].x = 10;
    auto reflected = renderCharacter(d, atlas, pose, options);
    check(reflected.pixelColor(3, 11) == QColor(Qt::blue),
          "nearest composition reflects visual branch");
    pose.bones[1].scaleX = 0;
    auto edgeOn = renderCharacter(d, atlas, pose, options);
    check(edgeOn.pixelColor(3, 11).alpha() == 0,
          "edge-on turn has no stale texture");
    pose.bones[1].scaleX = 1;
    QQuickWindow window;
    window.setColor(Qt::transparent);
    window.resize(64, 64);
    CharacterItem item(d, atlas, window.contentItem());
    pose.bones[1].x = 10;
    item.setPose(pose);
    window.show();
    auto frame = [&] {
      QElapsedTimer timer;
      timer.start();
      while (timer.elapsed() < 100)
        app.processEvents();
      return window.grabWindow();
    };
    auto direct = frame();
    check(!direct.isNull(), "scene graph capture");
    check(direct.pixelColor(11, 11) == QColor(Qt::blue),
          "direct scene graph attachment ordering");
    pose.bones[1].scaleX = -1;
    item.setPose(pose);
    direct = frame();
    check(direct.pixelColor(3, 11) == QColor(Qt::blue),
          "retained scene graph reflects visual branch");
    pose.bones[1].scaleX = 1;
    pose.bones[1].x = 30;
    item.setPose(pose);
    direct = frame();
    check(direct.pixelColor(11, 11).alpha() == 0,
          "retained direct nodes update pose");
    item.setMode(RenderMode::Composited);
    direct = frame();
    check(direct.pixelColor(31, 11) == QColor(Qt::blue),
          "composited scene graph updates texture");
    item.setVisible(false);
    CharacterItem extendedItem(stretched,limbAtlas,window.contentItem());
    extendedItem.setPose(longLeg);
    direct=frame();
    check(direct.pixelColor(28,11)==QColor(Qt::red),
          "retained renderer omits extended limb artwork");
    extendedItem.setMode(RenderMode::Composited);
    direct=frame();
    check(direct.pixelColor(28,11)==QColor(Qt::red),
          "composited renderer omits extended limb artwork");
    extendedItem.setVisible(false);
    CharacterItem blinkingItem(blink,atlas,window.contentItem());
    blinkingItem.setPose(openEyes);
    direct=frame();
    check(direct.pixelColor(18,18)==QColor(Qt::blue),
          "retained open eye artwork missing");
    blinkingItem.setPose(closedEyes);
    direct=frame();
    check(direct.pixelColor(18,18).alpha()==0,
          "retained renderer ignores eye openness-only changes");
    blinkingItem.setMode(RenderMode::Composited);
    direct=frame();
    check(direct.pixelColor(18,18).alpha()==0,
          "composited renderer ignores closed eyes");
    blinkingItem.setPose(openEyes);
    direct=frame();
    check(direct.pixelColor(18,18)==QColor(Qt::blue),
          "composited renderer ignores reopening eyes");
    std::cout << "character render tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}
