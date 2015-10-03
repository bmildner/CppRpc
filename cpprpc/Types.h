#ifndef CPPRPC_TYPES_H
#define CPPRPC_TYPES_H

#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <boost/serialization/access.hpp>

namespace CppRpc
{
  inline namespace V1
  {

    enum class InterfaceMode { Client, Server };

    using Name = std::string;

    using Byte = std::uint8_t;

    using Buffer = std::vector<Byte>;

    class Version
    {
      public:
        std::uint32_t Major;
        std::uint32_t Minor;

      private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int /*version*/)
        {
          ar & Major;
          ar & Minor;
        }

    };

    // forward declaration
    class Interface;

  }  // namespace V1
}  // namespace CppRpc

#endif
