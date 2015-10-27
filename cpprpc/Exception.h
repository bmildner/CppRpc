#ifndef CPPRPC_EXCEPTION_H
#define CPPRPC_EXCEPTION_H

#pragma once

#include <exception>


namespace CppRpc
{
  inline namespace V1
  {

// TODO: warning seems odd, investigate if this actually is hinting to a problem here!
// Constructor of abstract class 'Exception' ignores initializer for virtual base class 'std::exception'
// we have to globally disable it form here on as it occures at the point of usage of ExceptionImpl<>!
#pragma warning(disable: 4589) 

    class ExceptionInterface
    {
      public:
        virtual ~ExceptionInterface() noexcept = default;

        virtual const char* what() const = 0;
    };

    struct Exception : virtual std::exception, ExceptionInterface {};


    struct LocalException  : Exception {};
    struct RemoteException : Exception {};

    struct LibraryVersionMissmatch  : LocalException {};
    struct FunctionAlreadyRegistred : LocalException {};
    struct UnknownInterface         : LocalException {};
    struct UnknownFunction          : LocalException {};
    struct UnknownInterfaceMode     : LocalException {};

    struct UnknowRemoteException : RemoteException {};

    namespace Detail
    {

      template <typename Interface>
      class ExceptionImpl : public Interface, public virtual std::exception
      {
        public:
          template <typename T>
          explicit ExceptionImpl(T&& what)
          : Interface(), m_What(std::forward<T>(what))
          {}

          virtual const char* what() const override
          {
            return m_What.c_str();
          }

        private:
          static_assert(std::is_base_of<CppRpc::V1::Exception, Interface>::value, "<Interface> needs to be derived from Configuration::Exception!");

          std::string m_What;
      };

    }  // namespace Detail

  }  // namespace V1
}  // namespace CppRpc

#endif
