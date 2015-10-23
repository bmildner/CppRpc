#include "cpprpc/Transport.h"

#include <cassert>
#include <chrono>

#include "cpprpc/Exception.h"

namespace CppRpc
{
  inline namespace V1
  {

    LocalDummyTransport::LocalDummyTransport()
    : m_Mutex(), m_ClientToServerQueueCondVar(), m_ServerToClientQueueCondVar(), m_ClientToServerQueue(), m_ServerToClientQueue(), 
      m_Server(*this), m_Client(*this)
    {
    }

    void LocalDummyTransport::ClientSend(const Buffer & data)
    {
      Lock lock(m_Mutex);

      m_ClientToServerQueue.push_back(data);

      m_ClientToServerQueueCondVar.notify_one();
    }

    void LocalDummyTransport::ServerSend(const Buffer & data)
    {
      Lock lock(m_Mutex);

      m_ServerToClientQueue.push_back(data);

      m_ServerToClientQueueCondVar.notify_one();
    }

    bool LocalDummyTransport::ClientReceive(Buffer& data)
    {
      Lock lock(m_Mutex);

      if (m_ServerToClientQueueCondVar.wait_for(lock, std::chrono::milliseconds(Timeout), [this] { return !m_ServerToClientQueue.empty(); }))
      {
        data = m_ServerToClientQueue.front();

        m_ServerToClientQueue.pop_front();

        return true;
      }
      else
      {
        return false;
      }
    }

    bool LocalDummyTransport::ServerReceive(Buffer& data)
    {
      Lock lock(m_Mutex);

      if (m_ClientToServerQueueCondVar.wait_for(lock, std::chrono::milliseconds(Timeout), [this] { return !m_ClientToServerQueue.empty(); }))
      {
        data = m_ClientToServerQueue.front();

        m_ClientToServerQueue.pop_front();

        return true;
      }
      else
      {
        return false;
      }
    }

  }  // namespace V1
}  // namespace CppRpc
