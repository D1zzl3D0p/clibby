#include "support_surface.h"
#include <algorithm>
#include <cmath>
namespace pets::character {
std::optional<SurfaceSegment> usableSurface(const SupportSurface& s,double x,
    double left,double right,double now) {
  if(s.id.empty() || !std::isfinite(s.topY) || !std::isfinite(x) ||
     !std::isfinite(left) || !std::isfinite(right) || left>right ||
     !std::isfinite(now) || !std::isfinite(s.observedAt) ||
     !std::isfinite(s.maxAge) || s.maxAge<=0 || now<s.observedAt || now-s.observedAt>s.maxAge) return {};
  for(const auto& span:s.segments)
    if(std::isfinite(span.minimum) && std::isfinite(span.maximum) &&
       span.minimum<=x+left && x+right<=span.maximum)
      return SurfaceSegment{span.minimum-left,span.maximum-right};
  return {};
}
std::optional<SupportSurface> descendingSurface(const std::vector<SupportSurface>& ss,
    double x,double left,double right,double previous,double next,double now) {
  if(!std::isfinite(previous) || !std::isfinite(next) || next<=previous) return {};
  std::optional<SupportSurface> result;
  for(const auto& s:ss)
    if(previous<=s.topY && next>=s.topY && usableSurface(s,x,left,right,now) &&
       (!result || s.topY<result->topY)) result=s;
  return result;
}
std::vector<SurfaceSegment> subtractCovered(SurfaceSegment edge,std::vector<SurfaceSegment> covers) {
  std::vector<SurfaceSegment> result;
  if(!std::isfinite(edge.minimum)||!std::isfinite(edge.maximum)||edge.maximum<=edge.minimum) return result;
  covers.erase(std::remove_if(covers.begin(),covers.end(),[](const auto& s){return !std::isfinite(s.minimum)||!std::isfinite(s.maximum)||s.maximum<=s.minimum;}),covers.end());
  std::sort(covers.begin(),covers.end(),[](auto a,auto b){return a.minimum<b.minimum;});
  double start=edge.minimum;
  for(const auto& cover:covers) {
    if(cover.maximum<=start || cover.minimum>=edge.maximum) continue;
    if(cover.minimum>start) result.push_back({start,std::min(cover.minimum,edge.maximum)});
    start=std::max(start,cover.maximum);
    if(start>=edge.maximum) break;
  }
  if(start<edge.maximum) result.push_back({start,edge.maximum});
  return result;
}
std::optional<SurfaceDelta> surfaceMovement(const SupportSurface& old,const SupportSurface& next,
    double x,double left,double right,double now,double maximum) {
  if(old.id!=next.id || old.segments.empty() || next.segments.empty()) return {};
  // Translation is identified using the first visible segment only when the
  // entire segmentation translates. Occlusion changes never drag a contact.
  if(old.segments.size()!=next.segments.size()) return {};
  const double dx=next.segments.front().minimum-old.segments.front().minimum;
  const double dy=next.topY-old.topY;
  if(!std::isfinite(dx)||!std::isfinite(dy)||!std::isfinite(maximum)||maximum<0||std::hypot(dx,dy)>maximum) return {};
  for(size_t i=0;i<old.segments.size();++i)
    if(std::abs(next.segments[i].minimum-old.segments[i].minimum-dx)>.5 ||
       std::abs(next.segments[i].maximum-old.segments[i].maximum-dx)>.5) return {};
  if(!usableSurface(next,x+dx,left,right,now)) return {};
  return SurfaceDelta{dx,dy};
}
}
