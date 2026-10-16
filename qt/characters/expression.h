#pragma once
#include "motion3d.h"
#include <cstdint>
#include <memory>
namespace pets::character {
enum class Role { Head, EarLeft, EarRight, Tail, WristLeft, WristRight, Count };
constexpr std::uint32_t roleBit(Role role) {
  return static_cast<unsigned>(role)<static_cast<unsigned>(Role::Count)
      ? (std::uint32_t{1} << static_cast<unsigned>(role)) : 0;
}
// Latest-value orientation goals in radians: x pitch, y yaw, z tilt.
// Realized values advance only in step(); clear returns smoothly to rest.
class ExpressionMotion {
public:
  explicit ExpressionMotion(std::uint32_t capabilities=0);
  ~ExpressionMotion();
  ExpressionMotion(const ExpressionMotion &);
  ExpressionMotion &operator=(const ExpressionMotion &);
  ExpressionMotion(ExpressionMotion &&) noexcept;
  ExpressionMotion &operator=(ExpressionMotion &&) noexcept;
  bool supports(Role) const;
  bool goal(Role, pets::motion::Vec3);
  bool impulse(Role, pets::motion::Vec3);
  bool look(pets::motion::Vec3);
  bool clear(Role);
  void clear();
  void reset();
  void step(double dt);
  bool settled() const;
  pets::motion::Vec3 value(Role) const;
private:
  struct State;
  std::unique_ptr<State> state_;
};
} // namespace pets::character
