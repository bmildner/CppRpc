#include "cpprpc/Dispatcher.h"

#include "cpprpc/Interface.h"

namespace CppRpc
{
  inline namespace V1
  {
    namespace Detail
    {

      FunctionDispatchHeader::FunctionDispatchHeader(const CppRpc::V1::Interface& interface, const Name& functionName)
      : m_LibraryVersion(CppRpcVersion), m_Interface(interface.GetName()), m_Version(interface.GetVersion()), m_Function(functionName)
      {
      }

    }  // namespace Detail
  }  // namespace V1
}  // namespace CppRpc
