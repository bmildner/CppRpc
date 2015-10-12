#include "cpprpc/Dispatcher.h"

#include <utility>

#include <boost/format.hpp>

#include "cpprpc/Interface.h"
#include "cpprpc/Exception.h"


namespace CppRpc
{
  inline namespace V1
  {
    namespace Detail
    {

      FunctionDispatchHeader::FunctionDispatchHeader(const CppRpc::V1::Interface& interface, const Name& functionName)
      : m_Interface(interface.GetName()), m_Version(interface.GetVersion()), m_Function(functionName)
      {
      }

    }  // namespace Detail

    void Dispatcher::RegisterFunctionImplementation(const Interface& interface, const Name& name, FunctionImplementation implementation)
    {
      InterfaceIdentity interfaceIdentity = {interface.GetName(), interface.GetVersion()};

      // insert interface or find interface in map
      auto result = m_Interfaces.emplace(interfaceIdentity, Functions());


      // where we able to insert the new function or did it already exist?
      if (!result.first->second.emplace(name, implementation).second)
      {
        throw Detail::ExceptionImpl<FunctionAlreadyRegistred>((boost::format("Function \"%1%:%2%::%3%\" already registerd") % interface.GetName() % interface.GetVersion().str() % name).str());
      }
    }

  }  // namespace V1
}  // namespace CppRpc
