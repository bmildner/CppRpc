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

    void Dispatcher::DeregisterFunctionImplementation(const Interface& interface, const Name& name)
    {
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

  }  // namespace V1
}  // namespace CppRpc
