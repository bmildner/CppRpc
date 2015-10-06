#ifndef CPPRPC_DISPATCHER_H
#define CPPRPC_DISPATCHER_H

#pragma once

#include <string>

#include "cpprpc/Types.h"


namespace CppRpc
{
  inline namespace V1
  {
    class Interface;

    namespace Detail
    {
      struct FunctionDispatchHeader
      {
        FunctionDispatchHeader(const Interface& interface, const Name& functionName);

        Name    m_Interface;
        Version m_Version;
        Name    m_Function;
      };
    }

    class Dispatcher
    {
      public:
        using FunctionDispatchHeader = Detail::FunctionDispatchHeader;

    };

    using DefaultDispatcher = Dispatcher;

  }  // namespace V1
}  // namespace CppRpc

#endif

