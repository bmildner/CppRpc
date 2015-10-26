#ifndef CPPRPC_INTERFACE_H
#define CPPRPC_INTERFACE_H

#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <cassert>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Function.h"
#include "cpprpc/Marshaller.h"
#include "cpprpc/Transport.h"
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{
  
  inline namespace V1
  {

    template <InterfaceMode Mode, template <InterfaceMode> class Dispatcher = CppRpc::V1::Dispatcher>
    class Interface
    {
      public:
        using DispatcherHandle = DispatcherHandle<Mode>;

        Interface(Transport<Mode>& transport, const Name& name, Version version = {1, 0})
        : Interface(MakeDispatcherHandle(transport), name, version)
        {}        

        Interface(const DispatcherHandle& dispatcher, const Name& name, Version version = {1, 0})
        : m_Name(name), m_Version(version), m_Dispatcher(dispatcher)
        {
          assert(m_Dispatcher);
        }

        virtual ~Interface() noexcept = default;

        const Name& GetName() const { return m_Name; }
        const Version& GetVersion() const { return m_Version; }

        const DispatcherHandle& GetDispatcher() const { return m_Dispatcher; }


        template <typename T>
        using Function = Detail::Function<T, Mode, Dispatcher>;

      private:
        Name             m_Name;
        Version          m_Version;
        DispatcherHandle m_Dispatcher;
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
