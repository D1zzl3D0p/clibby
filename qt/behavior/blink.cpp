#include "blink.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace pets::behavior {
namespace {
constexpr double closeTime=.055, holdEnd=.080, duration=.190, frameTime=1.0/60;
std::optional<double> future(double now,double delay) {
    double value=now+delay;
    if(value<=now) value=std::nextafter(now,std::numeric_limits<double>::infinity());
    return std::isfinite(value)?std::optional<double>(value):std::nullopt;
}
double smooth(double value) {
    value=std::clamp(value,0.0,1.0);
    return value*value*(3-2*value);
}
}
Blink::Blink(std::uint32_t seed):seed_(seed),rng_(seed) { reset(0); }
void Blink::reset(double now) {
    if(!std::isfinite(now)) now=0;
    rng_.seed(seed_); lastNow_=now; elapsed_=0; openness_=1; suppressed_=false;
    wait(now);
}
void Blink::wait(double now) {
    active_=false; elapsed_=0; openness_=1;
    deadline_=future(now,std::uniform_real_distribution<double>(7,16)(rng_));
}
void Blink::frame(double now) {
    double boundary=elapsed_<closeTime?closeTime:elapsed_<holdEnd?holdEnd:duration;
    deadline_=future(now,std::min(frameTime,boundary-elapsed_));
}
bool Blink::advance(double now,bool suppressed) {
    if(!std::isfinite(now)) return false;
    const double before=openness_;
    if(suppressed) {
        suppressed_=true; deadline_.reset(); lastNow_=now;
        return false;
    }
    if(suppressed_) {
        suppressed_=false; lastNow_=now;
        if(active_) frame(now); else wait(now);
        return openness_!=before;
    }
    if(now<lastNow_) {
        lastNow_=now;
        if(active_) frame(now); else wait(now);
        return openness_!=before;
    }
    const double delta=now-lastNow_;
    lastNow_=now;
    if(!active_) {
        if(!deadline_ || now<*deadline_) return false;
        // Sleeping hosts do not replay a missed blink.
        const double late=now-*deadline_;
        if(late>=duration) { wait(now); return openness_!=before; }
        active_=true; elapsed_=late;
    } else elapsed_+=delta;
    if(elapsed_>=duration-1e-10) wait(now);
    else {
        openness_=elapsed_<closeTime?1-smooth(elapsed_/closeTime):
            elapsed_<holdEnd?0:smooth((elapsed_-holdEnd)/(duration-holdEnd));
        frame(now);
    }
    return openness_!=before;
}
}
