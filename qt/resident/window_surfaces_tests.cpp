#include "window_surfaces.h"
#include <QGuiApplication>
#include <QRasterWindow>
#include <QPainter>
#include <QTimer>
#include <cmath>
#include <iostream>
class PerchWindow : public QRasterWindow {
  void paintEvent(QPaintEvent*) override {QPainter painter(this); painter.fillRect(QRect(QPoint(),size()),QColor(210,220,235));}
};
int main(int argc,char** argv) {
  QGuiApplication app(argc,argv);
  if(argc==2 && QString::fromLocal8Bit(argv[1])=="--fixture") {
    PerchWindow window; window.setTitle("Clibby perch check");
    window.setGeometry(480,300,600,240); window.show();
    QTimer::singleShot(250,&app,[&]{window.raise(); window.requestActivate(); std::cout<<"READY\n"<<std::flush;});
    QTimer::singleShot(120000,&app,&QCoreApplication::quit);
    return app.exec();
  }
  const auto snapshot=pets::observeWindowSurfaces(10);
  if(QGuiApplication::platformName()!="cocoa") {
    if(snapshot.available || !snapshot.surfaces.empty()) return 1;
    std::cout<<"PASS unsupported/offscreen provider is unavailable\n";
    return 0;
  }
  if(!snapshot.available) { std::cout<<"SKIP native window geometry unavailable\n"; return 77; }
  for(const auto& surface:snapshot.surfaces) {
    if(surface.id.empty()||!std::isfinite(surface.topY)||surface.observedAt!=10||surface.segments.empty()) return 1;
    for(const auto& segment:surface.segments)
      if(!std::isfinite(segment.minimum)||!std::isfinite(segment.maximum)||segment.maximum<=segment.minimum) return 1;
  }
  if(argc==2 && QString::fromLocal8Bit(argv[1])=="--describe")
    for(const auto& surface:snapshot.surfaces) {
      std::cout<<"edge top="<<surface.topY;
      for(const auto& segment:surface.segments)std::cout<<" span="<<segment.minimum<<","<<segment.maximum;
      std::cout<<"\n";
    }
  std::cout<<"PASS native geometry query available; "<<snapshot.surfaces.size()<<" visible usable window edges\n";
  return 0;
}
