#ifndef CPPRPC_DISPATCHER_H
#define CPPRPC_DISPATCHER_H

#pragma once

#include <string>
#include <functional>
#include <map>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <boost/format.hpp>
#include <boost/noncopyable.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Transport.h"

namespace CppRpc
{
  inline namespace V1
  {
    template <InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
    class Interface;

    namespace Detail
    {
      
      struct RemoteFunctionCall
      {
        RemoteFunctionCall()
        {}

        template <InterfaceMode Mode, template <InterfaceMode> class Dispatcher>
        RemoteFunctionCall(const Interface<Mode, Dispatcher>& interface, const Name& functionName, const Buffer& paramData)
        : m_InterfaceName(interface.GetName()), m_InterfaceVersion(interface.GetVersion()), m_FunctionName(functionName), m_ParameterData(paramData)
        {
        }

        RemoteFunctionCall(RemoteFunctionCall&&) = default;
        RemoteFunctionCall& operator=(RemoteFunctionCall&&) = default;

        Name    m_InterfaceName;
        Version m_InterfaceVersion;
        Name    m_FunctionName;
        Buffer  m_ParameterData;
      };
    }


    // TODO: probably seperate Client and Server implmentation like with class Function (using FunctionImpl)
    template <InterfaceMode Mode>
    class Dispatcher : boost::noncopyable
    {
      public:
        using RemoteFunctionCall = Detail::RemoteFunctionCall;

        using FunctionImplementation = std::function<Buffer(const Buffer&)>;

        Dispatcher(Transport<Mode>& transport);

        ~Dispatcher();

        void RegisterFunctionImplementation(const Interface<Mode, CppRpc::V1::Dispatcher>& interface, const Name& name, FunctionImplementation implementation);
        void DeregisterFunctionImplementation(const Interface<Mode, CppRpc::V1::Dispatcher>& interface, const Name& name);

        Buffer CallRemoteFunction(const Buffer& callData);  // client side call into Transport
        Buffer DoFunctionCall(const Buffer& callData);  // server side call into function implementation

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

        Transport<Mode>& m_Transport;  // TODO: change to shared_ptr


        using Thread = std::thread;
        using Mutex  = std::recursive_mutex;
        using Lock   = std::unique_lock<Mutex>;

        Thread m_ServerThread;
        Mutex  m_Mutex;

        bool m_StopServerThread;

        static void ServerThread(Dispatcher<Mode>* dispatcher);
    };  // class Dispatcher


    template <InterfaceMode Mode>
    Dispatcher<Mode>::Dispatcher(Transport<Mode>& transport)
    : m_Transport(transport), m_ServerThread(), m_Mutex(), m_StopServerThread(false)
    {
#pragma warning(suppress: 4127)  // conditional expression is constant
      if (Mode == InterfaceMode::Server)
      {
        m_ServerThread = Thread(ServerThread, this);
      }
    }

    template <InterfaceMode Mode>
    Dispatcher<Mode>::~Dispatcher()
    {
      {
        Lock lock(m_Mutex);

        m_StopServerThread = true;
      }

      if (m_ServerThread.joinable())
      {
        m_ServerThread.join();
      }
    }

    template <InterfaceMode Mode>
    void Dispatcher<Mode>::RegisterFunctionImplementation(const Interface<Mode, CppRpc::V1::Dispatcher>& interface, const Name& name, FunctionImplementation implementation)
    {
      InterfaceIdentity interfaceIdentity = {interface.GetName(), interface.GetVersion()};

      Lock lock(m_Mutex);

      // insert interface or find interface in map
      auto result = m_Interfaces.emplace(interfaceIdentity, Functions());


      // where we able to insert the new function or did it already exist?
      if (!result.first->second.emplace(name, implementation).second)
      {
        throw Detail::ExceptionImpl<FunctionAlreadyRegistred>((boost::format("Function \"%1%:%2%::%3%\" already registerd") % interface.GetName() % interface.GetVersion().str() % name).str());
      }
    }

    template <InterfaceMode Mode>
    void Dispatcher<Mode>::DeregisterFunctionImplementation(const Interface<Mode, CppRpc::V1::Dispatcher>& interface, const Name& name)
    {
      Lock lock(m_Mutex);

      // find interface
      auto interfaceIter = m_Interfaces.find({interface.GetName(), interface.GetVersion()});

      if (interfaceIter == m_Interfaces.end())
      {
        throw Detail::ExceptionImpl<UnknownInterface>((boost::format("Unknown Interface \"%1%:%2%\"") % interface.GetName() % interface.GetVersion().str()).str());
      }

      // find function
      auto functionIter = interfaceIter->second.find(name);

      if (functionIter == interfaceIter->second.end())
      {
        throw Detail::ExceptionImpl<UnknownFunction>((boost::format("Unknown Function \"%1%:%2%::%3%\"") % interface.GetName() % interface.GetVersion().str() % name).str());
      }

      // remove function
      interfaceIter->second.erase(functionIter);

      // remove Interface if empty
      if (interfaceIter->second.empty())
      {
        m_Interfaces.erase(interfaceIter);
      }
    }

    template <InterfaceMode Mode>
    Buffer Dispatcher<Mode>::CallRemoteFunction(const Buffer& callData)
    {
      m_Transport.Send(callData);

      Buffer returnData;

      // TODO: handle timeouts and stuff ...
      while (!m_Transport.Receive(returnData));
      
      return returnData;      
    }

    template <InterfaceMode Mode>
    Buffer Dispatcher<Mode>::DoFunctionCall(const Buffer& callData)
    {
      Detail::RemoteFunctionCall functionHeader = Marshaller<CppRpc::V1::Dispatcher>::DeserializeFunctionDispatchHeader(callData);

      FunctionImplementation functionImpl;

      {
        Lock lock(m_Mutex);

        auto interfaceIter = m_Interfaces.find({functionHeader.m_InterfaceName, functionHeader.m_InterfaceVersion});

        if (interfaceIter != m_Interfaces.end())
        {
          auto functionIter = interfaceIter->second.find(functionHeader.m_FunctionName);

          if (functionIter != interfaceIter->second.end())
          {
            // found function implementation
            functionImpl = functionIter->second;
          }
          else
          {
            // TODO: throw error
            return Buffer();
          }
        }
        else
        {
          // TODO: thorw error
          return Buffer();
        }
      }

      assert(functionImpl);

      // call function implementation
      return functionImpl(functionHeader.m_ParameterData);
    }

    template <InterfaceMode Mode>
    void Dispatcher<Mode>::ServerThread(Dispatcher<Mode>* dispatcher)
    {
      assert(dispatcher != nullptr);

      Buffer callData;

      for (;;)
      {        
        if (dispatcher->m_Transport.Receive(callData))  // non-blocking, timeout mandatory in Transport::Receive() !
        {
          assert(callData.size() >= sizeof(Detail::RemoteFunctionCall));

          dispatcher->m_Transport.Send(dispatcher->DoFunctionCall(callData));
        }

        Lock lock(dispatcher->m_Mutex);

        if (dispatcher->m_StopServerThread)
        {
          return;
        }
      }
    }

    template <InterfaceMode Mode>
    using DispatcherHandle = std::shared_ptr<Dispatcher<Mode>>;

    template <template <InterfaceMode> class Transport>
    DispatcherHandle<Transport::Mode> MakeDispatcherHandle(Transport& transport)
    {
      return std::make_shared<DispatcherHandle<Transport::Mode>::element_type>(transport);
    }

//    template <InterfaceMode Mode>
//    using DefaultDispatcher = Dispatcher<Mode>;

  }  // namespace V1
}  // namespace CppRpc

#endif

