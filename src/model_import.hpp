#pragma once

#undef _MSC_VER
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "file.hpp"
#include "gpu/vulkan/buffer.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "memory.hpp"
#include "string.hpp"

struct Vertex {
  Vec3f position;
  Vec3f normal;
  Vec2f uv;
};

struct StandardMesh3d {
  Vertex *vertices  = nullptr;
  u32 vertices_size = 0;
  u32 *indexes      = nullptr;
  u32 indexes_size  = 0;
};

StandardMesh3d load_mesh(String filename, StackAllocator *allocator)
{
  File file = read_file(filename, &tmp_allocator);

  const aiScene *assimp_scene = aiImportFileFromMemory(
      (char *)file.data.data, file.data.size, aiProcess_Triangulate, nullptr);
  if (!assimp_scene || assimp_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !assimp_scene->mRootNode) {
    error("Assimp error loading file: ", filename);
    return {nullptr, 0};
  }

  aiNode *node = assimp_scene->mRootNode;
  while (node->mNumMeshes == 0) {
    node = node->mChildren[0];
  }

  i32 assimp_mesh_idx = node->mMeshes[0];
  aiMesh *assimp_mesh = assimp_scene->mMeshes[assimp_mesh_idx];

  StandardMesh3d mesh;
  mesh.vertices_size = assimp_mesh->mNumVertices;
  mesh.vertices =
      (Vertex *)allocator->alloc(mesh.vertices_size * sizeof(Vertex)).data;

  for (u32 i = 0; i < mesh.vertices_size; i++) {
    mesh.vertices[i].position.x = assimp_mesh->mVertices[i].x;
    mesh.vertices[i].position.y = assimp_mesh->mVertices[i].y;
    mesh.vertices[i].position.z = assimp_mesh->mVertices[i].z;
  }
  for (u32 i = 0; i < mesh.vertices_size; i++) {
    mesh.vertices[i].normal.x = assimp_mesh->mNormals[i].x;
    mesh.vertices[i].normal.y = assimp_mesh->mNormals[i].y;
    mesh.vertices[i].normal.z = assimp_mesh->mNormals[i].z;
  }
  for (u32 i = 0; i < mesh.vertices_size; i++) {
    mesh.vertices[i].uv.x = assimp_mesh->mTextureCoords[0][i].x;
    mesh.vertices[i].uv.y = assimp_mesh->mTextureCoords[0][i].y;
  }

  mesh.indexes_size = assimp_mesh->mNumFaces * 3;
  mesh.indexes = (u32 *)allocator->alloc(mesh.indexes_size * sizeof(u32)).data;
  for (u32 i = 0; i < assimp_mesh->mNumFaces; i++) {
    mesh.indexes[i * 3 + 0] = assimp_mesh->mFaces[i].mIndices[0];
    mesh.indexes[i * 3 + 1] = assimp_mesh->mFaces[i].mIndices[1];
    mesh.indexes[i * 3 + 2] = assimp_mesh->mFaces[i].mIndices[2];
  }

  return mesh;
}
