#ifndef CPPRPC_FUNCTION_H
#define CPPRPC_FUNCTION_H

#pragma once

#include <type_traits>
#include <cassert>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/variant/get.hpp>
#include <boost/format.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"
#include "cpprpc/Exception.h"


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
            // TODO: do not use default marshaller ...

            // serialize function call
            Buffer callData = DefaultMarshaller<Dispatcher>::SerializeFunctionCall<ParamTypes>(m_Interface, m_Name, std::forward<Arguments>(arguments)...);
          
            // do remote function call
            Buffer returnData = m_Interface.GetDispatcher()->CallRemoteFunction(callData);

            // de-serialize result (return value or exception)
            Detail::RemoteCallResult<ReturnType> result = DefaultMarshaller<Dispatcher>::DeserializeReturnValue<Detail::RemoteCallResult<ReturnType>>(returnData);

            assert(!result.empty());

            // check if exception data was returned instead of a return value
            if (result.which() != 0)
            {
              const Detail::RemoteExceptionData& exceptionData = boost::get<Detail::RemoteExceptionData>(result);

              // TODO: add handling for registred exception types
              // throw de-serialize exception
              throw Detail::ExceptionImpl<UnknowRemoteException>((boost::format("Exception type: \"%1%\", what: \"%2%\"") % exceptionData.m_Name % exceptionData.m_What).str());
            }

            // return result
            return ReturnValueHelper<ReturnType>()(result);
          }

        private:

          // helper for void return type
          template <typename ReturnValue>
          struct ReturnValueHelper
          {
            template <typename RemoteCallResult>
            ReturnValue operator()(RemoteCallResult& result)
            {
              return std::move(boost::get<ReturnValue>(result));
            }
          };

          template <>
          struct ReturnValueHelper<void>
          {
            template <typename RemoteCallResult>
            void operator()(RemoteCallResult& /*result*/)
            {
            }
          };

      };  // class FunctionImpl<InterfaceMode::Client>

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
            m_Interface.GetDispatcher()->RegisterFunctionImplementation(m_Interface, m_Name, marshalledImplementation);
          }

          virtual ~FunctionImpl() noexcept override
          {
            try
            {
              m_Interface.GetDispatcher()->DeregisterFunctionImplementation(m_Interface, m_Name);
            }

            catch (...)
            {
              // TODO: add trace / logging
            }
          }

        private:
          std::function<T> m_Implementation;
      };    


      // TODO: explore possibility to add overloaded functions (new compound function class initialized with overloads?!?)
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

    }  // namespace  Detail
  }  // namespace V1
}  // namespace CppRpc

#endif
