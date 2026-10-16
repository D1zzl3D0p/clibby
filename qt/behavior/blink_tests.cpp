#include "blink.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using pets::behavior::Blink;
int main() {
    Blink a(42), b(42); a.reset(0); b.reset(0);
    double smallest=100, largest=0;
    for(int i=0;i<100;++i) {
        assert(a.nextDeadline()==b.nextDeadline());
        double due=*a.nextDeadline();
        assert(a.openness()==1 && !a.advance(due-.01));
        assert(*a.nextDeadline()==due); // No frame timer while quiet.
        a.advance(due); b.advance(due);
        assert(*a.nextDeadline()>due && *a.nextDeadline()<=due+1.0/60+.000001);
        assert(a.advance(due+.03)); b.advance(due+.03);
        assert(a.openness()>0 && a.openness()<1 && a.openness()==b.openness());
        a.advance(due+.06); b.advance(due+.06); assert(a.openness()==0);
        a.advance(due+.09); b.advance(due+.09); assert(a.openness()>0 && a.openness()<1);
        assert(a.advance(due+.19)); b.advance(due+.19); assert(a.openness()==1);
        double interval=*a.nextDeadline()-(due+.19);
        assert(interval>=7 && interval<=16);
        smallest=std::min(smallest,interval); largest=std::max(largest,interval);
    }
    assert(largest-smallest>5);
    a.reset(0); b.reset(0); assert(a.nextDeadline()==b.nextDeadline());
    assert(*a.nextDeadline()>=7 && *a.nextDeadline()<=16);
    const double due=*a.nextDeadline(); a.advance(due); a.advance(due+.03);
    double frozen=a.openness();
    assert(!a.advance(due+.04,true) && !a.nextDeadline());
    assert(!a.advance(due+1000,true) && a.openness()==frozen);
    assert(!a.advance(due+1001) && a.openness()==frozen && a.nextDeadline());
    a.advance(due+1001.03); assert(a.openness()==0);
    a.advance(due+1001.2); assert(a.openness()==1);
    a.advance(due+1001.3,true); assert(!a.nextDeadline());
    a.advance(due+2000); assert(a.openness()==1 && *a.nextDeadline()>=due+2007);
    a.advance(due+1e6); assert(a.openness()==1 && *a.nextDeadline()>due+1e6);
    a.reset(0); a.advance(*a.nextDeadline()); a.advance(*a.nextDeadline());
    frozen=a.openness(); a.advance(-100); assert(a.openness()==frozen && *a.nextDeadline()>-100);
    assert(!a.advance(std::numeric_limits<double>::quiet_NaN()));
    assert(!a.advance(std::numeric_limits<double>::infinity()));
    assert(a.openness()==frozen);
    // A suspended host or late wake never queues old blinks.
    a.reset(0); double lateDue=*a.nextDeadline();
    a.advance(lateDue+2); assert(a.openness()==1 && *a.nextDeadline()>=lateDue+9);
    a.reset(0); lateDue=*a.nextDeadline(); a.advance(lateDue+.03);
    assert(a.openness()<1 && a.advance(lateDue+1e6));
    assert(a.openness()==1 && *a.nextDeadline()>=lateDue+1e6+7);
    // Rewinding while quiet rebases to a future sparse wait.
    a.reset(100); assert(!a.advance(-100) && *a.nextDeadline()>=-93);
    a.reset(1e20); assert(a.openness()==1 && *a.nextDeadline()>1e20);
    a.reset(std::numeric_limits<double>::max()); assert(!a.nextDeadline());
    a.reset(std::numeric_limits<double>::quiet_NaN()); assert(std::isfinite(a.openness()));
    std::cout<<"blink tests passed\n";
}
