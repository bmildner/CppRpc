#ifndef CPPRPC_DISPATCHER_H
#define CPPRPC_DISPATCHER_H

#pragma once

#include <string>
#include <functional>
#include <map>
#include <utility>

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

        using FunctionImplementation = std::function<Buffer(const Buffer&)>;

        void RegisterFunctionImplementation(const Interface& interface, const Name& name, FunctionImplementation implementation);

        void DeregisterFunctionImplementation(const Interface& interface, const Name& name);

      private:
        
        using Functions = std::map<Name, FunctionImplementation>;

        struct InterfaceIdentity
        {
          Name    m_Name;
          Version m_Version;

          bool operator<(const InterfaceIdentity& other) const
          {
            return (m_Name < other.m_Name) || ((m_Name == other.m_Name) && (m_Version < other.m_Version));
          }
        };

        using Interfaces = std::map<InterfaceIdentity, Functions>;

        Interfaces m_Interfaces;
    };

    using DefaultDispatcher = Dispatcher;

  }  // namespace V1
}  // namespace CppRpc

#endif

