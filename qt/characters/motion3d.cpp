#include "motion3d.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::motion {
namespace {
Vec3 add(Vec3 a, Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
Vec3 sub(Vec3 a, Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 mul(Vec3 a, double s) { return {a.x*s,a.y*s,a.z*s}; }
double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
double length(Vec3 a) { return std::sqrt(dot(a,a)); }
bool finite(Vec3 p) { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
}
TwoLinkPose solveTwoLink(Vec3 origin, Vec3 target, double upper, double lower,
                         Vec3 bend, bool preserveTarget) {
 if(!finite(origin)||!finite(target)||!finite(bend)||!std::isfinite(upper)||
    !std::isfinite(lower)||upper<=0||lower<=0||upper>8192||lower>8192)
  throw std::runtime_error("invalid spatial two-link input");
 const Vec3 delta=sub(target,origin);
 const double raw=length(delta);
 if(!std::isfinite(raw))throw std::runtime_error("spatial target exceeds numeric range");
 const Vec3 axis=raw>1e-12?mul(delta,1/raw):Vec3{0,-1,0};
 const double epsilon=std::min(1e-9,std::min(upper,lower)*1e-6);
 if(preserveTarget) {
  // Smooth the full-extension singularity: retaining a small flex prevents
  // the knee's square-root bend from snapping when reach becomes elastic.
  // Both planar and spatial contact legs use the same narrow compliance band.
  const double authoredReach=upper+lower;
  const double compliantReach=.5*(authoredReach+raw+
      std::hypot(authoredReach-raw,authoredReach*.01));
  upper*=compliantReach/authoredReach;
  lower*=compliantReach/authoredReach;
  if(raw>upper+lower) {
   const double scale=raw/(upper+lower);
   upper*=scale; lower*=scale;
  } else if(raw<std::abs(upper-lower)) {
   // Extend the shorter link at the inner fold instead of moving the contact.
   if(upper>lower)lower=upper-raw;
   else upper=lower-raw;
  }
 }
 const double reach=preserveTarget?raw:
     std::clamp(raw,std::abs(upper-lower)+epsilon,upper+lower-epsilon);
 Vec3 across=sub(bend,mul(axis,dot(bend,axis)));
 if(length(across)<1e-12)across=sub(Vec3{1,0,0},mul(axis,axis.x));
 if(length(across)<1e-12)across=sub(Vec3{0,0,1},mul(axis,axis.z));
 across=mul(across,1/length(across));
 if(preserveTarget&&reach<1e-12)
  return {origin,add(origin,mul(across,upper)),target};
 const double along=(upper*upper-lower*lower+reach*reach)/(2*reach);
 const Vec3 knee=add(add(origin,mul(axis,along)),
                     mul(across,std::sqrt(std::max(0.,upper*upper-along*along))));
 return {origin,knee,preserveTarget?target:add(origin,mul(axis,reach))};
}
}
