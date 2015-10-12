#ifndef CPPRPC_FUNCTION_H
#define CPPRPC_FUNCTION_H

#pragma once

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{

  inline namespace V1
  {

    class Interface;

    template <typename T>
    class FunctionImplBase
    {
      public:
        FunctionImplBase(Interface& interface, const Name& name)
        : m_Name(name), m_Interface(interface)
        {}

        virtual ~FunctionImplBase() noexcept = default;

        const Name& GetName() const { return m_Name; }

      protected:
        using ReturnType = typename boost::function_types::result_type<T>::type;
        using ParamTypes = typename boost::function_types::parameter_types<T>::type;

        Name       m_Name;
        Interface& m_Interface;
    };


    template <typename T, InterfaceMode mode>
    class FunctionImpl : public FunctionImplBase<T>
    {
      static_assert(true, "unknown interface mode");
    };

    template <typename T>
    class FunctionImpl<T, InterfaceMode::Client> : public FunctionImplBase<T>
    {
      public:
        template <typename Implementation>
        FunctionImpl(Interface& interface, const Name& name, Implementation&& /*implementation*/)
        : FunctionImplBase(interface, name)
        {}

        virtual ~FunctionImpl() noexcept override = default;

        template <typename... Arguments>
        ReturnType operator()(Arguments&&... arguments)
        {
          Buffer callData = DefaultMarshaller::SerializeFunctionCall<ParamTypes>(m_Interface, m_Name, std::forward<Arguments>(arguments)...);
          
          Buffer returnData = DelaufTransport<InterfaceMode::Client>().TransferCall(callData);

          return DefaultMarshaller::DeserializeReturnValue<ReturnType>(returnData);
        }
    };

    template <typename T>
    class FunctionImpl<T, InterfaceMode::Server> : public FunctionImplBase<T>
    {
      public:
        template <typename Implementation>
        FunctionImpl(Interface& interface, const Name& name, Implementation&& implementation)
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


    template <typename T, InterfaceMode mode>
    class Function : public FunctionImpl<T, mode>
    {
      public:
        template <typename Implementation>
        Function(Interface& interface, const Name& name, Implementation&& implementation)
        : FunctionImpl(interface, name, std::forward<Implementation>(implementation))
        {}

        virtual ~Function() noexcept override = default;
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
