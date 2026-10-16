#include "support_surface.h"
#include <stdexcept>
#include <iostream>
using namespace pets::character;
void check(bool ok) { if(!ok) throw std::runtime_error("support surface contract failed"); }
int main() {
  SupportSurface edge{"window:7", 200, {{100, 180}, {220, 300}}, 10};
  check(bool(usableSurface(edge, 140, -20, 20, 10)));
  check(!usableSurface(edge, 200, -20, 20, 10));
  check(!usableSurface(edge, 110, -20, 20, 10));
  check(!usableSurface(edge, 140, -20, 20, 12));
  std::vector<SupportSurface> edges{edge, {"window:8", 250, {{100,300}},10}};
  check(!descendingSurface(edges,140,-20,20,240,190,10));
  auto landing=descendingSurface(edges,140,-20,20,190,280,10);
  check(landing && landing->id=="window:7");
  check(!descendingSurface(edges,140,-20,20,210,240,10));
  auto cut=subtractCovered({100,300},{{140,160},{150,220},{280,400}});
  check(cut.size()==2 && cut[0].minimum==100 && cut[0].maximum==140 && cut[1].minimum==220 && cut[1].maximum==280);
  auto moved=edge; moved.topY=215; moved.segments={{130,210},{250,330}};
  auto delta=surfaceMovement(edge,moved,140,-20,20,10);
  check(delta && delta->x==30 && delta->y==15);
  moved.topY=400;
  check(!surfaceMovement(edge,moved,140,-20,20,10));
  moved=edge; moved.id="window:9";
  check(!surfaceMovement(edge,moved,140,-20,20,10));
  std::cout<<"PASS support surface acquisition, coverage, freshness and movement\n";
}
