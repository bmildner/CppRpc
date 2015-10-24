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
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{
  
  inline namespace V1
  {

    template <InterfaceMode Mode, template <InterfaceMode> class Dispatcher = Dispatcher>
    class Interface
    {
      public:
        Interface(Transport<Mode>& transport, const Name& name, Version version = {1, 0})
        : m_Name(name), m_Version(version), m_Dispatcher(transport)
        {}        

        virtual ~Interface() noexcept = default;

        const Name& GetName() const { return m_Name; }
        const Version& GetVersion() const { return m_Version; }

        Dispatcher<Mode>& GetDispatcher() { return m_Dispatcher; }
        const Dispatcher<Mode>& GetDispatcher() const { return m_Dispatcher; }


        template <typename T>
        using Function = Detail::Function<T, Mode, Dispatcher>;

      private:
        Name             m_Name;
        Version          m_Version;
        Dispatcher<Mode> m_Dispatcher;  // TODO: use externally provided dispatcher!
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
