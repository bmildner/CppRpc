#include "cpprpc/Dispatcher.h"

#include "cpprpc/Interface.h"

namespace CppRpc
{
  inline namespace V1
  {
    namespace Detail
    {

      FunctionDispatchHeader::FunctionDispatchHeader(const CppRpc::V1::Interface& interface, const Name& functionName)
      : Interface(interface.GetName()), Version(interface.GetVersion()), Function(functionName)
      {
      }

    }  // namespace Detail
  }  // namespace V1
}  // namespace CppRpc
