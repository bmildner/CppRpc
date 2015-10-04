#ifndef CPPRPC_INTERFACE_H
#define CPPRPC_INTERFACE_H

#pragma once

#include <string>
#include <cstdint>
#include <functional>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Marshaller.h"
#include "cpprpc/Transport.h"

namespace CppRpc
{
  
  inline namespace V1
  {
  
    const Version CppRpcVersion = {1, 0};

    class Interface
    {
      public:
        Interface(const Name& name, Version version = {1, 0})
        : m_Name(name), m_Version(version)
        {}

        virtual ~Interface() = default;

        const Name& GetName() const {return m_Name;}
        const Version& GetVersion() const {return m_Version;}

      private:
        Name    m_Name;
        Version m_Version;
    };


    class FunctionImplBase
    {
      public:
        FunctionImplBase(const Interface& interface, const Name& name)
        : m_Name(name), m_Interface(interface)
        {}

        virtual ~FunctionImplBase() = default;

        const Name& GetName() const { return m_Name; }
        const Interface& GetInterface() const { return m_Interface; }

      private:
        Name             m_Name;
        const Interface& m_Interface;
    };


    template <typename T, InterfaceMode mode,
              typename ReturnType = typename boost::function_types::result_type<T>::type, 
              typename ParamTypes = typename boost::function_types::parameter_types<T>::type>
    class FunctionImpl : public FunctionImplBase
    {
      static_assert(true, "unknown interface mode");
    };

    template <typename T,
    typename ReturnType, typename ParamTypes>
    class FunctionImpl<T, InterfaceMode::Client, ReturnType, ParamTypes> : public FunctionImplBase
    {
      public:
        using RetType = ReturnType;

        template <typename Implementation>
        FunctionImpl(const Interface& interface, const Name& name, Implementation&& /*implementation*/)
        : FunctionImplBase(interface, name)
        {}

        template <typename... Arguments>
        ReturnType operator()(Arguments&&... arguments)
        {
          DefaultMarshaller marshaller;

          Buffer callData = marshaller.SerializeFunctionCall<ParamTypes>(GetInterface(), GetName(), std::forward<Arguments>(arguments)...);
          
          Buffer returnData = DelaufTransport<InterfaceMode::Client>().TransferCall(callData);

          return marshaller.DeserializeReturnValue<ReturnType>(returnData);
        }
    };

    template <typename T, 
              typename ReturnType, typename ParamTypes>
    class FunctionImpl<T, InterfaceMode::Server, ReturnType, ParamTypes> : public FunctionImplBase
    {
      public:
        using RetType = ReturnType;

        template <typename Implementation>
        FunctionImpl(const Interface& interface, const Name& name, Implementation&& implementation)
        : FunctionImplBase(interface, name), m_Implementation(std::forward<Implementation>(implementation))
        {}

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

    };

  }  // namespace V1
}  // namespace CppRpc

#endif
