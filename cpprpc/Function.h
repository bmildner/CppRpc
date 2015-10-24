#ifndef CPPRPC_FUNCTION_H
#define CPPRPC_FUNCTION_H

#pragma once

#include <type_traits>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{

  inline namespace V1
  {

    template <InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
    class Interface;

    namespace Detail
    {

      template <typename T, InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
      class FunctionImplBase
      {
        public:
          FunctionImplBase(Interface<Mode, Dispatcher>& interface, const Name& name)
          : m_Name(name), m_Interface(interface)
          {}

          virtual ~FunctionImplBase() noexcept = default;

          const Name& GetName() const { return m_Name; }

          static_assert(std::is_function<T>::value, "T must be a function type (like \"void(int)\")");

        protected:
          // TODO: check for not supportet types (pointers ?!?)
          // TODO: wrap in-out params
          using ReturnType = typename boost::function_types::result_type<T>::type;
          using ParamTypes = typename boost::function_types::parameter_types<T>::type;

          Name                         m_Name;
          Interface<Mode, Dispatcher>& m_Interface;
      };


      template <typename T, InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
      class FunctionImpl : public FunctionImplBase<T, Mode, Dispatcher>
      {
        static_assert(true, "unknown interface mode");
      };

      // function implementation for client mode
      template <typename T, template <InterfaceMode> class Dispatcher>
      class FunctionImpl<T, InterfaceMode::Client, Dispatcher> : public FunctionImplBase<T, InterfaceMode::Client, Dispatcher>
      {
        public:
          template <typename Implementation>
          FunctionImpl(Interface<InterfaceMode::Client, Dispatcher>& interface, const Name& name, Implementation&& /*implementation*/)
          : FunctionImplBase(interface, name)
          {}

          virtual ~FunctionImpl() noexcept override = default;

          template <typename... Arguments>
          ReturnType operator()(Arguments&&... arguments)
          {
            // TODO: do not use default dipatcher ...
            Buffer callData = DefaultMarshaller<Dispatcher>::SerializeFunctionCall<ParamTypes>(m_Interface, m_Name, std::forward<Arguments>(arguments)...);
          
            Buffer returnData = m_Interface.GetDispatcher().CallRemoteFunction(callData);

            return DefaultMarshaller<Dispatcher>::DeserializeReturnValue<ReturnType>(returnData);
          }
      };

      // function implementation for server mode
      template <typename T, template <InterfaceMode> class Dispatcher>
      class FunctionImpl<T, InterfaceMode::Server, Dispatcher> : public FunctionImplBase<T, InterfaceMode::Server, Dispatcher>
      {
        public:
          template <typename Implementation>
          FunctionImpl(Interface<InterfaceMode::Server, Dispatcher>& interface, const Name& name, Implementation&& implementation)
          : FunctionImplBase(interface, name), m_Implementation(std::forward<Implementation>(implementation))
          {
            auto marshalledImplementation = [this] (const Buffer& paramData) -> Buffer
              { 
                // TODO: do not use default dipatcher ...
                return DefaultMarshaller<Dispatcher>::DeserializeAndExecuteFunctionCall<ReturnType, ParamTypes>(paramData, m_Implementation);
              };

            // register function
            m_Interface.GetDispatcher().RegisterFunctionImplementation(m_Interface, m_Name, marshalledImplementation);
          }

          virtual ~FunctionImpl() noexcept override
          {
            try
            {
              m_Interface.GetDispatcher().DeregisterFunctionImplementation(m_Interface, m_Name);
            }

            catch (...)
            {
              // TODO: add trace / logging
            }
          }

        private:
          std::function<T> m_Implementation;
      };

    }  // namespace  Detail

    template <typename T, InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
    class Function : public Detail::FunctionImpl<T, Mode, Dispatcher>
    {
      public:
        template <typename Implementation>
        Function(Interface<Mode, Dispatcher>& interface, const Name& name, Implementation&& implementation)
        : FunctionImpl(interface, name, std::forward<Implementation>(implementation))
        {}

        virtual ~Function() noexcept override = default;        
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
