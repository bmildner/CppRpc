#ifndef CPPRPC_TRANSPORT_H
#define CPPRPC_TRANSPORT_H

#pragma once

#include <functional>

#include "cpprpc/Types.h"

namespace CppRpc
{
  inline namespace V1
  {
    
    template <InterfaceMode mode>
    class Transport
    {
      public:
        template <typename Callback>
        explicit Transport(Callback&& callback)
        : m_ServerCallback(std::forward<Callback>(callback))
        {}

        // TODO: remove
        Transport()
        : Transport(nullptr)
        {}

        Buffer TransferCall(const Buffer& callData)
        {
          return m_ServerCallback(callData);
        }

        virtual ~Transport() = default;

      private:
        std::function<Buffer(const Buffer&)> m_ServerCallback;
    };

    template < InterfaceMode mode>
    using DelaufTransport = Transport<mode>;

  }  // namespace V1
}  // namespace CppRpc

#endif

