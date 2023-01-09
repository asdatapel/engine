#pragma once

#include <iostream>
#include <stdio.h>

#include "string.hpp"

std::ostream& operator<<(std::ostream& out, String str)
{
  out.write((char*)str.data, str.size);
  return out;
}

template <class... Args>
void fatal(Args... args)
{
  (std::cout << ... << args) << std::endl;
  abort();
}

template <class... Args>
void warning(Args... args)
{
  (std::cout << ... << args) << "\n" << std::endl;
}

template <class... Args>
void info(Args... args)
{
  (std::cout << ... << args) << "\n";
}