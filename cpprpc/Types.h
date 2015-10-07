#ifndef CPPRPC_TYPES_H
#define CPPRPC_TYPES_H

#pragma once

#include <string>
#include <vector>
#include <cstdint>


namespace CppRpc
{
  inline namespace V1
  {

    enum class InterfaceMode { Client, Server };

    using Name = std::string;

    using Byte = std::uint8_t;

    using Buffer = std::vector<Byte>;

    struct Version
    {
      std::uint16_t m_Major;
      std::uint16_t m_Minor;
    };

    const std::uint8_t LibraryVersionV1 = 1;  // boost::serialization versions may only be 8bit wide ...
    const std::uint8_t LibraryVersion = LibraryVersionV1;

  }  // namespace V1
}  // namespace CppRpc

#endif
