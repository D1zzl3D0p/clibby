#include "window_surfaces.h"
#include <QGuiApplication>
#import <CoreGraphics/CoreGraphics.h>
#include <unistd.h>
#include <cmath>
#include <algorithm>
namespace pets {
WindowSurfaceSnapshot observeWindowSurfaces(double now) {
  if(QGuiApplication::platformName()!="cocoa" || !std::isfinite(now)) return {};
  CFArrayRef windows=CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly |
                                               kCGWindowListExcludeDesktopElements,kCGNullWindowID);
  if(!windows) return {};
  WindowSurfaceSnapshot result; result.available=true;
  std::vector<CGRect> covers;
  const auto count=std::min<CFIndex>(CFArrayGetCount(windows),256);
  for(CFIndex i=0;i<count;++i) {
    auto dict=static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows,i));
    int pid=0,layer=0,number=0; double alpha=0; CGRect rect{};
    auto numeric=[&](CFStringRef key,CFNumberType type,void* out){
      auto v=CFDictionaryGetValue(dict,key);
      return v && CFGetTypeID(v)==CFNumberGetTypeID() && CFNumberGetValue(static_cast<CFNumberRef>(v),type,out);
    };
    auto bounds=CFDictionaryGetValue(dict,kCGWindowBounds);
    if(!numeric(kCGWindowOwnerPID,kCFNumberIntType,&pid)||pid==getpid() ||
       !numeric(kCGWindowLayer,kCFNumberIntType,&layer)||layer<0 ||
       !numeric(kCGWindowNumber,kCFNumberIntType,&number)||
       !numeric(kCGWindowAlpha,kCFNumberDoubleType,&alpha)||alpha<=.01 ||
       !bounds||CFGetTypeID(bounds)!=CFDictionaryGetTypeID()||
       !CGRectMakeWithDictionaryRepresentation(static_cast<CFDictionaryRef>(bounds),&rect)||
       !std::isfinite(rect.origin.x)||!std::isfinite(rect.origin.y)||
       !std::isfinite(rect.size.width)||!std::isfinite(rect.size.height)||
       rect.size.width<=0||rect.size.height<=0) continue;
    // On-screen-only excludes minimized and other-Space windows. Positive
    // layers can obscure an edge but do not offer ordinary document perches.
    if(layer==0 && rect.size.width>=32 && rect.size.height>=32) {
      std::vector<character::SurfaceSegment> covered;
      for(auto occluder:covers)
        if(occluder.origin.y<=rect.origin.y+1 && CGRectGetMaxY(occluder)>rect.origin.y)
          covered.push_back({occluder.origin.x,CGRectGetMaxX(occluder)});
      auto segments=character::subtractCovered({rect.origin.x,CGRectGetMaxX(rect)},std::move(covered));
      if(!segments.empty()) result.surfaces.push_back({"mac-window:"+std::to_string(pid)+":"+std::to_string(number),rect.origin.y,std::move(segments),now});
    }
    covers.push_back(rect);
  }
  CFRelease(windows);
  return result;
}
}
