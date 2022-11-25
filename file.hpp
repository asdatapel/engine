#pragma once

#include <fstream>
#include <iostream>

#include "memory.hpp"
#include "string.hpp"

struct File {
  String path;
  String data;
  Mem mem;
};

File read_file(String path, Allocator *allocator)
{
  Temp tmp;

  Mem null_terminated_path = tmp.alloc(path.size + 1);
  memcpy(null_terminated_path.data, path.data, path.size);
  null_terminated_path.data[path.size] = '\0';

  std::ifstream in_stream((char *)null_terminated_path.data, std::ios::binary | std::ios::ate);
  u64 file_size = in_stream.tellg();

  File file;
  file.mem = allocator->alloc(path.size + file_size);

  file.path.data = file.mem.data;
  file.path.size = path.size;
  memcpy(file.path.data, path.data, path.size);

  file.data.data = file.mem.data + file.path.size;
  file.data.size = file_size;

  in_stream.seekg(0, std::ios::beg);
  in_stream.read((char *)file.data.data, file.data.size);
  in_stream.close();

  return file;
}