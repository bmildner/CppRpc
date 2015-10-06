#ifndef CPPRPC_FUNCTION_H
#define CPPRPC_FUNCTION_H

#pragma once


#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"


namespace CppRpc
{

  inline namespace V1
  {

    class Interface;

    template <typename T>
    class FunctionImplBase
    {
      public:
        FunctionImplBase(const Interface& interface, const Name& name)
        : m_Name(name), m_Interface(interface)
        {}

        virtual ~FunctionImplBase() = default;

        const Name& GetName() const { return m_Name; }
        const Interface& GetInterface() const { return m_Interface; }

      protected:
        using ReturnType = typename boost::function_types::result_type<T>::type;
        using ParamTypes = typename boost::function_types::parameter_types<T>::type;

      private:
        Name             m_Name;
        const Interface& m_Interface;
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
        using RetType = ReturnType;

        template <typename Implementation>
        FunctionImpl(const Interface& interface, const Name& name, Implementation&& /*implementation*/)
        : FunctionImplBase(interface, name)
        {}

        virtual ~FunctionImpl() override = default;

        template <typename... Arguments>
        ReturnType operator()(Arguments&&... arguments)
        {
          DefaultMarshaller marshaller;

          Buffer callData = marshaller.SerializeFunctionCall<ParamTypes>(GetInterface(), GetName(), std::forward<Arguments>(arguments)...);
          
          Buffer returnData = DelaufTransport<InterfaceMode::Client>().TransferCall(callData);

          return marshaller.DeserializeReturnValue<ReturnType>(returnData);
        }
    };

    template <typename T>
    class FunctionImpl<T, InterfaceMode::Server> : public FunctionImplBase<T>
    {
      public:
        using RetType = ReturnType;

        template <typename Implementation>
        FunctionImpl(const Interface& interface, const Name& name, Implementation&& implementation)
        : FunctionImplBase(interface, name), m_Implementation(std::forward<Implementation>(implementation))
        {}

        virtual ~FunctionImpl() override = default;

        // TODO: remove, should not be needed, there should only be server internal calls to the implementations
        template <typename... Arguments>
        ReturnType operator()(Arguments&&... arguments)
        {
          return m_Implementation(std::forward<Arguments>(arguments)...);
        }

      private:
        std::function<T> m_Implementation;
    };


    template <typename T, InterfaceMode mode>
    class Function : public FunctionImpl<T, mode>
    {
      public:
        template <typename Implementation>
        Function(const Interface& interface, const Name& name, Implementation&& implementation)
        : FunctionImpl(interface, name, std::forward<Implementation>(implementation))
        {}

        virtual ~Function() override = default;
    };

  }  // namespace V1
}  // namespace CppRpc

#endif
