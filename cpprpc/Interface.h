#ifndef CPPRPC_INTERFACE_H
#define CPPRPC_INTERFACE_H

#pragma once

#include <string>
#include <cstdint>
#include <functional>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Function.h"
#include "cpprpc/Marshaller.h"
#include "cpprpc/Transport.h"

namespace CppRpc
{
  
  inline namespace V1
  {

    class Interface
    {
      public:
        Interface(const Name& name, Version version = {1, 0})
        : m_Name(name), m_Version(version)
        {}

        virtual ~Interface() noexcept = default;

        const Name& GetName() const {return m_Name;}
        const Version& GetVersion() const {return m_Version;}

      private:
        Name    m_Name;
        Version m_Version;
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
