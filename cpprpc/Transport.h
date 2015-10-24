#ifndef CPPRPC_TRANSPORT_H
#define CPPRPC_TRANSPORT_H

#pragma once

#include <list>
#include <mutex>
#include <condition_variable>

#include "cpprpc/Types.h"

namespace CppRpc
{
  inline namespace V1
  {
            
    template <InterfaceMode Mode>
    class Transport
    {
      public:
        Transport()
        {}

        virtual ~Transport() noexcept = default;

        // TODO: replace or add io-stream interface!
        virtual void Send(const Buffer& data) = 0;
        virtual bool Receive(Buffer& data) = 0;

      protected:
    };


    class LocalDummyTransport
    {
        template <InterfaceMode Mode>
        class LocalDummyTransportImpl : public Transport<Mode>
        {
          public:
            LocalDummyTransportImpl(LocalDummyTransport& parent)
            : Transport(), m_Parent(parent)
            {}

            virtual void Send(const Buffer& data) override
            {
#pragma warning(suppress: 4127)  // conditional expression is constant
              if (Mode == InterfaceMode::Client)
              {
                m_Parent.ClientSend(data);
              }
              else
              {
                m_Parent.ServerSend(data);
              }
            }

            virtual bool Receive(Buffer& data) override
            {
#pragma warning(suppress: 4127)  // conditional expression is constant
              if (Mode == InterfaceMode::Client)
              {
                return m_Parent.ClientReceive(data);
              }
              else
              {
                return m_Parent.ServerReceive(data);
              }
            }

          private:
            LocalDummyTransport& m_Parent;
        };

        using ClientImplementation = LocalDummyTransportImpl<InterfaceMode::Client>;
        using ServerImplementation = LocalDummyTransportImpl<InterfaceMode::Server>;

      public:
        LocalDummyTransport();

        ClientImplementation& GetClientTransport()
        {
          return m_Client;
        }

        ServerImplementation& GetServerTransport()
        {
          return m_Server;
        }

      protected:
        // TODO: probably make timeout an parameter!?
        static const unsigned Timeout = 250;  // in milliseconds

        using Mutex = std::recursive_mutex;
        using Lock = std::unique_lock<Mutex>;
        using ConditionVariable = std::condition_variable_any;

        using Queue = std::list<Buffer>;

        Mutex m_Mutex;
        ConditionVariable m_ClientToServerQueueCondVar;
        ConditionVariable m_ServerToClientQueueCondVar;

        Queue m_ClientToServerQueue;
        Queue m_ServerToClientQueue;

        ServerImplementation m_Server;
        ClientImplementation m_Client;

        void ClientSend(const Buffer& data);
        bool ClientReceive(Buffer& data);

        void ServerSend(const Buffer& data);
        bool ServerReceive(Buffer& data);

    };

  }  // namespace V1
}  // namespace CppRpc

#endif
