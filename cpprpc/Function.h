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

    template <InterfaceMode Mode>
    class Interface;

    namespace Detail
    {

      template <typename T, InterfaceMode Mode>
      class FunctionImplBase
      {
        public:
          FunctionImplBase(Interface<Mode>& interface, const Name& name)
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

          Name             m_Name;
          Interface<Mode>& m_Interface;
      };


      template <typename T, InterfaceMode Mode>
      class FunctionImpl : public FunctionImplBase<T, Mode>
      {
        static_assert(true, "unknown interface mode");
      };

      template <typename T>
      class FunctionImpl<T, InterfaceMode::Client> : public FunctionImplBase<T, InterfaceMode::Client>
      {
        public:
          template <typename Implementation>
          FunctionImpl(Interface<InterfaceMode::Client>& interface, const Name& name, Implementation&& /*implementation*/)
          : FunctionImplBase(interface, name)
          {}

          virtual ~FunctionImpl() noexcept override = default;

          template <typename... Arguments>
          ReturnType operator()(Arguments&&... arguments)
          {
            Buffer callData = DefaultMarshaller::SerializeFunctionCall<ParamTypes>(m_Interface, m_Name, std::forward<Arguments>(arguments)...);
          
            Buffer returnData = m_Interface.GetDispatcher().CallRemoteFunction(callData);

            return DefaultMarshaller::DeserializeReturnValue<ReturnType>(returnData);
          }
      };

      template <typename T>
      class FunctionImpl<T, InterfaceMode::Server> : public FunctionImplBase<T, InterfaceMode::Server>
      {
        public:
          template <typename Implementation>
          FunctionImpl(Interface<InterfaceMode::Server>& interface, const Name& name, Implementation&& implementation)
          : FunctionImplBase(interface, name), m_Implementation(std::forward<Implementation>(implementation))
          {
            auto marshalledImplementation = [this] (const Buffer& paramData) -> Buffer
              { 
                return DefaultMarshaller::DeserializeAndExecuteFunctionCall<ReturnType, ParamTypes>(paramData, m_Implementation);
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

    template <typename T, InterfaceMode Mode>
    class Function : public Detail::FunctionImpl<T, Mode>
    {
      public:
        template <typename Implementation>
        Function(Interface<Mode>& interface, const Name& name, Implementation&& implementation)
        : FunctionImpl(interface, name, std::forward<Implementation>(implementation))
        {}

        virtual ~Function() noexcept override = default;        
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
