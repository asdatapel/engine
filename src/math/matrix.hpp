#pragma once

#include <cmath>

#include "math/vector.hpp"
#include "types.hpp"

template <u32 WIDTH, u32 HEIGHT, typename T>
struct Matrix {
  T data[WIDTH * HEIGHT] = {};

  Matrix(){};
  Matrix(T value)
  {
    for (i32 i = 0; i < WIDTH; i++) {
      this->data[(i * WIDTH) + i] = value;
    }
  }
  T* operator[](u32 i) { return data + (WIDTH * i); }
};

template <u32 NEW_WIDTH, u32 NEW_HEIGHT, u32 SHARED_SIZE, typename T>
Matrix<NEW_WIDTH, NEW_HEIGHT, T> operator*(Matrix<SHARED_SIZE, NEW_HEIGHT, T> a,
                                           Matrix<NEW_WIDTH, SHARED_SIZE, T> b)
{
  Matrix<NEW_HEIGHT, NEW_WIDTH, T> out;
  for (i32 y = 0; y < NEW_HEIGHT; y++) {
    for (i32 x = 0; x < NEW_WIDTH; x++) {
      T sum = 0;
      for (i32 k = 0; k < SHARED_SIZE; k++) {
        sum += a[k][y] * b[x][k];
      }
      out[x][y] = sum;
    }
  }

  return out;
}

using Mat4f = Matrix<4, 4, f32>;

Mat4f look_at(Vec3f eye, Vec3f at, Vec3f up)
{
  Vec3f zaxis = normalize(at - eye);
  Vec3f xaxis = normalize(cross(zaxis, up));
  Vec3f yaxis = cross(xaxis, zaxis);

  // zaxis = -zaxis;

  // Mat4f viewMatrix = {Vec4f{xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, eye)},
  //                     Vec4f{yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, eye)},
  //                     Vec4f{zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, eye)},
  //                     Vec4f{0, 0, 0, 1}};

  Mat4f mat;
  mat[0][0] = xaxis.x;
  mat[1][0] = xaxis.y;
  mat[2][0] = xaxis.z;
  mat[3][0] = -dot(xaxis, eye);
  mat[0][1] = yaxis.x;
  mat[1][1] = yaxis.y;
  mat[2][1] = yaxis.z;
  mat[3][1] = -dot(yaxis, eye);
  mat[0][2] = zaxis.x;
  mat[1][2] = zaxis.y;
  mat[2][2] = zaxis.z;
  mat[3][2] = -dot(zaxis, eye);
  mat[0][3] = 0;
  mat[1][3] = 0;
  mat[2][3] = 0;
  mat[3][3] = 1;

  return mat;
}
Mat4f translate(Mat4f mat, Vec3f t)
{
  mat[3][0] += t.x;
  mat[3][1] += t.y;
  mat[3][2] += t.z;

  return mat;
}

Mat4f perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  f32 fov_tan  = 1.f / tanf(fov / 2);
  f32 far_norm = far / (far - near);

  Mat4f m;
  m[0][0] = (1 / aspect) * fov_tan;
  m[1][1] = fov_tan;
  m[2][2] = far_norm;
  m[3][2] = -far_norm * near;
  m[2][3] = 1;

  return m;
}