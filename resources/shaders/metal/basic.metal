typedef uchar  u8;
typedef ushort u16;
typedef uint   u32;
typedef long   u64;
typedef char   i8;
typedef short  i16;
typedef int    i32;
typedef long   i64;
typedef half   f16;
typedef float  f32;

typedef packed_float2 Vec2f;
typedef packed_float3 Vec3f;
typedef packed_float4 Vec4f;
//
//
//template <typename T>
//struct Vec2 {
//  T x, y;
//
//  Vec2() {}
//  Vec2(T x, T y)
//  {
//    this->x = x;
//    this->y = y;
//  }
//
//  template <typename T2>
//  Vec2(constant Vec2<T2> &other)
//  {
//    this->x = other.x;
//    this->y = other.y;
//  }
//
//  f32 len() { return sqrt(x * x + y * y); }
//};
//
//template <typename T>
//inline Vec2<T> operator+(constant Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  return {lhs.x + rhs.x, lhs.y + rhs.y};
//}
//template <typename T>
//inline Vec2<T> operator-(constant Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  return {lhs.x - rhs.x, lhs.y - rhs.y};
//}
//template <typename T>
//inline Vec2<T> operator-(constant Vec2<T> &v)
//{
//  return {-v.x, -v.y};
//}
//template <typename T>
//inline void operator+=(device Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  lhs = lhs + rhs;
//}
//template <typename T>
//inline void operator-=(device Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  lhs = lhs - rhs;
//}
//template <typename T>
//inline Vec2<T> operator*(constant Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  return {lhs.x * rhs.x, lhs.y * rhs.y};
//}
//template <typename T>
//inline Vec2<T> operator*(constant Vec2<T> &lhs, constant f32 &rhs)
//{
//  return lhs * Vec2<T>{rhs, rhs};
//}
//template <typename T>
//inline Vec2<T> operator*(constant f32 &lhs, constant Vec2<T> &rhs)
//{
//  return rhs * lhs;
//}
//template <typename T>
//inline Vec2<T> operator/(constant Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  return {lhs.x / rhs.x, lhs.y / rhs.y};
//}
//template <typename T>
//inline Vec2<T> operator/(constant Vec2<T> &lhs, constant f32 &rhs)
//{
//  return lhs / Vec2<T>{rhs, rhs};
//}
//template <typename T>
//inline Vec2<T> operator/(constant f32 &lhs, constant Vec2<T> &rhs)
//{
//  return rhs / lhs;
//}
//template <typename T>
//inline f32 dot(constant Vec2<T> &lhs, constant Vec2<T> &rhs)
//{
//  return lhs.x * rhs.x + lhs.y * rhs.y;
//}
//template <typename T>
//Vec2<T> min(Vec2<T> a, Vec2<T> b)
//{
//  return {fminf(a.x, b.x), fminf(a.y, b.y)};
//}
//template <typename T>
//Vec2<T> max(Vec2<T> a, Vec2<T> b)
//{
//  return {fmaxf(a.x, b.x), fmaxf(a.y, b.y)};
//}
//template <typename T>
//Vec2<T> normalize(Vec2<T> v)
//{
//  float len = sqrt(v.x * v.x + v.y * v.y);
//  return {v.x / len, v.y / len};
//}
//template <typename T>
//Vec2<T> clamp(Vec2<T> val, Vec2<T> min, Vec2<T> max)
//{
//  return {clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y)};
//}
//template <typename T>
//Vec2<T> abs(Vec2<T> a)
//{
//  return {abs(a.x), abs(a.y)};
//}
//
//// Vec3
//template <typename T>
//struct Vec3 {
//  union {
//    struct {
//      T x, y, z;
//    };
//    T values[3];
//  };
//
//  float len() { return sqrt(x * x + y * y + z * z); }
//  float operator[](i32 i) { return values[i]; }
//};
//
//template <typename T>
//inline Vec3<T> operator+(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
//}
//template <typename T>
//inline Vec3<T> operator-(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
//}
//template <typename T>
//inline Vec3<T> operator-(constant Vec3<T> &v)
//{
//  return {-v.x, -v.y, -v.z};
//}
//template <typename T>
//inline Vec3<T> operator*(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
//}
//template <typename T>
//inline Vec3<T> operator*(constant Vec3<T> &lhs, constant float &rhs)
//{
//  return lhs * Vec3<T>{rhs, rhs, rhs};
//}
//template <typename T>
//inline Vec3<T> operator*(constant float &lhs, constant Vec3<T> &rhs)
//{
//  return rhs * lhs;
//}
//template <typename T>
//inline Vec3<T> operator/(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
//}
//template <typename T>
//inline Vec3<T> operator/(constant float &lhs, constant Vec3<T> &rhs)
//{
//  return {lhs / rhs.x, lhs / rhs.y, lhs / rhs.z};
//}
//template <typename T>
//inline Vec3<T> operator/(constant Vec3<T> &lhs, constant float &rhs)
//{
//  return {lhs.x / rhs, lhs.y / rhs, lhs.z / rhs};
//}
//template <typename T>
//inline float dot(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
//}
//template <typename T>
//inline Vec3<T> cross(constant Vec3<T> &lhs, constant Vec3<T> &rhs)
//{
//  return {
//      lhs.y * rhs.z - lhs.z * rhs.y,
//      lhs.z * rhs.x - lhs.x * rhs.z,
//      lhs.x * rhs.y - lhs.y * rhs.x,
//  };
//}
//template <typename T>
//Vec3<T> normalize(Vec3<T> v)
//{
//  float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
//  return {v.x / len, v.y / len, v.z / len};
//}
//template <typename T>
//Vec3<T> min(Vec3<T> a, Vec3<T> b)
//{
//  return {fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)};
//}
//template <typename T>
//Vec3<T> max(Vec3<T> a, Vec3<T> b)
//{
//  return {fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z)};
//}
//
//template <typename T>
//struct Vec4 {
//  union {
//    struct {
//      T x, y, z, w;
//    };
//    T values[4];
//  };
//
//  float len() { return sqrt(x * x + y * y + z * z + w * w); }
//  float operator[](i32 i) { return values[i]; }
//};
//
//typedef Vec2<i32> Vec2i;
//typedef Vec2<u32> Vec2u;
//typedef Vec2<f32> Vec2f;
//typedef Vec2<bool> Vec2b;
//typedef Vec3<f32> Vec3f;
//typedef Vec4<f32> Vec4f;
//