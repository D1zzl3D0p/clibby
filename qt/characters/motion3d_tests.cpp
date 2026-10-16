#include "motion3d.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::motion;
static double distance(Vec3 a,Vec3 b) { return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z); }
static void check(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
int main() {
 try {
  for(const auto target : {Vec3{30,-40,20},Vec3{0,-2,0},Vec3{0,0,0},Vec3{4,-8,3}}) {
   const auto pose=solveTwoLink({0,0,0},target,10,7,{0,0,1},true);
   check(distance(pose.end,target)<1e-10,"pinned target was projected into reach");
   check(distance(pose.origin,pose.knee)>=10-1e-10,"upper link was shortened");
   check(distance(pose.knee,pose.end)>=7-1e-10,"lower link was shortened");
  }
  const auto rigid=solveTwoLink({0,0,0},{30,-40,20},10,7,{0,0,1});
  check(std::abs(distance(rigid.origin,rigid.knee)-10)<1e-8&&
        std::abs(distance(rigid.knee,rigid.end)-7)<1e-8,"default rigid solver changed");
  const auto inReach=solveTwoLink({0,0,0},{4,-8,3},10,7,{0,0,1},true);
  check(distance(inReach.origin,inReach.knee)>=10-1e-10&&
        distance(inReach.origin,inReach.knee)<10.001&&
        distance(inReach.knee,inReach.end)>=7-1e-10&&
        distance(inReach.knee,inReach.end)<7.001,"reachable compliance exceeds its narrow transition");
  const auto near=solveTwoLink({0,0,0},{0,-16.9999,0},10,7,{0,0,1},true);
  const auto far=solveTwoLink({0,0,0},{0,-17.0001,0},10,7,{0,0,1},true);
  check(distance(near.knee,far.knee)<.001,
        "pinned knee snaps at the rigid/elastic reach boundary");
  std::cout<<"spatial pinned target checks passed\n";
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
