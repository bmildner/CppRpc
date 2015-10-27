#ifndef CPPRPC_MARSHALLER_H
#define CPPRPC_MARSHALLER_H

#pragma once

#include <sstream>
#include <type_traits>
#include <cassert>

#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/variant/variant.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/version.hpp>
#pragma warning(push)
#pragma warning(disable: 4100)  // boost/serialization/collections_load_imp.hpp(67): warning C4100: 'item_version': unreferenced formal parameter
#include <boost/serialization/vector.hpp>
#pragma warning(pop)
#include <boost/serialization/variant.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"
#include "cpprpc/Exception.h"


namespace CppRpc
{
  inline namespace V1
  {
    namespace Detail
    {

      template<class Archive>
      inline void serialize(Archive& ar, RemoteFunctionCall& funcDispHeader, const unsigned int version)
      {
        if (version == LibraryVersionV1)
        {
          ar & funcDispHeader.m_InterfaceName;
          ar & funcDispHeader.m_InterfaceVersion;
          ar & funcDispHeader.m_FunctionName;
          ar & funcDispHeader.m_ParameterData;
        }
        else
        {
          throw Detail::ExceptionImpl<LibraryVersionMissmatch>("Version of class RemoteFunctionCall not equal to expected library version");
        }
      }


      struct RemoteExceptionData
      {
        std::string m_Name;
        std::string m_What;
      };

      template<class Archive>
      inline void serialize(Archive& ar, RemoteExceptionData& exceptionData, const unsigned int version)
      {
        if (version == LibraryVersionV1)
        {
          ar & exceptionData.m_Name;
          ar & exceptionData.m_What;
        }
        else
        {
          throw Detail::ExceptionImpl<LibraryVersionMissmatch>("Version of class RemoteExceptionData not equal to expected library version");
        }
      }


      template <typename ReturnType>
      struct RemoteCallResultHelper
      {
        using Type = boost::variant<ReturnType, RemoteExceptionData>;
      };

      template <>
      struct RemoteCallResultHelper<void>
      {
        using Type = boost::variant<bool, RemoteExceptionData>;
      };

      template <typename ReturnType>
      using RemoteCallResult = typename RemoteCallResultHelper<ReturnType>::Type;

    }  // namespace Detail

    template<class Archive>
    inline void serialize(Archive& ar, Version& ver, const unsigned int version)
    {
      if (version == LibraryVersionV1)
      {
        ar & ver.m_Major;
        ar & ver.m_Minor;
      }
      else
      {
        throw Detail::ExceptionImpl<LibraryVersionMissmatch>("Version of class Version not equal to expected library version");
      }
    }


    // TODO: extract de-/serializer into (template) parameter
    template <template <InterfaceMode> class Dispatcher>
    class Marshaller
    {
      private:
        using OStream = std::ostringstream;
        using OArchive = boost::archive::text_oarchive;

        using IStream = std::istringstream;
        using IArchive = boost::archive::text_iarchive;

      public:

        template <typename ArgumentTypes, InterfaceMode Mode, typename... Arguments>
        static Buffer SerializeFunctionCall(const Interface<Mode, Dispatcher>& interface, const Name& functionName, Arguments&&... arguments);

        template <typename ReturnType>
        static ReturnType DeserializeReturnValue(const Buffer& buffer);

        static Detail::RemoteFunctionCall DeserializeFunctionDispatchHeader(const Buffer& data);

        template <typename ReturnType, typename ArgumentTypes, typename Implementaion>
        static Buffer DeserializeAndExecuteFunctionCall(const Buffer& paramData, Implementaion& implementation);

      private:

        template <typename ArgumentTypes, typename Argument, typename... RemainingArguments>
        static void SerializeArguments(OArchive& archive, Argument&& argument, RemainingArguments&&... remainingArguments);

        // sentinal overload to end recursion
        template <typename ArgumentTypes>
        static void SerializeArguments(OArchive& /*archive*/);


        template <typename ReturnType, typename ArgumentTypes, 
                  typename RemoteCallResult = Detail::RemoteCallResult<ReturnType>, std::size_t ArgumentCount = boost::mpl::size<ArgumentTypes>::value>
        struct FunctionCallHelper
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value > 0, "");

          template <typename Implementaion, typename... Arguments>
          void operator()(IArchive& iarchive, OArchive& oarchive, Implementaion& implementation, Arguments&&... arguments)
          {
            using ParameterType = std::remove_reference_t<boost::mpl::front<ArgumentTypes>::type>;

            // deserialize parameter
            ParameterType param = Deserialize<std::remove_const_t<ParameterType>>(iarchive);

            // deserialize remaining parameters OR do function call
            FunctionCallHelper<ReturnType, boost::mpl::pop_front<ArgumentTypes>::type>()(iarchive, oarchive, implementation, std::forward<Arguments>(arguments)..., param);
          }
        };

        // spezialisation for ReturnType != void AND no more parameters
        template <typename ReturnType, typename ArgumentTypes, typename RemoteCallResult>
        struct FunctionCallHelper<ReturnType, ArgumentTypes, RemoteCallResult, 0>
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");
          static_assert(!std::is_void<ReturnType>::value, "");

          template <typename Implementaion, typename... Arguments>
          void operator()(IArchive& /*archive*/, OArchive& oarchive, Implementaion& implementation, Arguments&&... arguments)
          {
            try
            {
              // TODO: try to seperate exception thrown by implementation (+ argument passing) and exception from serialization code?
              // call function implementation AND serialize result (avoid named temporary instance of ReturnType!)
              Serialize<RemoteCallResult>(oarchive, implementation(std::forward<Arguments>(arguments)...));
            }
            catch (...)
            {              
              HandleException<RemoteCallResult>(oarchive);
            }            
          }
        };

        // spezialisation for ReturnType == void AND no more parameters
        template <typename ArgumentTypes, typename RemoteCallResult>
        struct FunctionCallHelper<void, ArgumentTypes, RemoteCallResult, 0>
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");

          template <typename Implementaion, typename... Arguments>
          void operator()(IArchive& /*archive*/, OArchive& oarchive, Implementaion& implementation, Arguments&&... arguments)
          {
            // call function implementation
            try
            {
              implementation(std::forward<Arguments>(arguments)...);
              Serialize<RemoteCallResult>(oarchive, RemoteCallResult());
            }
            catch (...)
            {
              HandleException<RemoteCallResult>(oarchive);              
            }
          }
        };

        template <typename RemoteCallResult>
        static void HandleException(OArchive& oarchive)
        {
          try
          {
            throw;
          }
          
          catch (const std::exception& e)
          {
            // TODO: add support for serialization of registred exception types
            Serialize<RemoteCallResult>(oarchive, Detail::RemoteExceptionData({typeid(e).name(), e.what()}));
          }

          catch (...)
          {
            Serialize<RemoteCallResult>(oarchive, Detail::RemoteExceptionData({"Unknown exception type", ""}));
          }
        }

        template <typename T>
        static void Serialize(OArchive& archive, const T& data)
        {
          archive << data;
        }


        template <typename T>
        static T Deserialize(IArchive& archive)
        {
          return DeserializeHelper<T>::Deserialize(archive);
        }

        template <typename T>
        struct DeserializeHelper
        {
          static T Deserialize(IArchive& archive)
          {
            T t;

            archive >> t;

            return t;
          }
        };

        // spezialisation for T = void
        template <>
        struct DeserializeHelper<void>
        {
          static void Deserialize(IArchive& /*archive*/)
          {}
        };

    };  // class Marshaller

    template <template <InterfaceMode> class Dispatcher>
    template <typename ArgumentTypes, InterfaceMode Mode, typename... Arguments>
    Buffer Marshaller<Dispatcher>::SerializeFunctionCall(const Interface<Mode, Dispatcher>& interface, const Name& functionName, Arguments&&... arguments)
    {
      // check number of arguments (ArgumentTypes vs Arguments)
      static_assert(boost::mpl::size<ArgumentTypes>::value == sizeof...(arguments), "invalid number of arguments supplied");

      OStream stream;

      {
        OArchive archive(stream);

        // serialize arguments
        SerializeArguments<ArgumentTypes>(archive, std::forward<Arguments>(arguments)...);
      }

      auto str = stream.str();

      // reset output stream
      stream = OStream();

      {
        OArchive archive(stream);

        // serialize function call
        Serialize(archive, Dispatcher<Mode>::RemoteFunctionCall(interface, functionName, Buffer(str.data(), str.data() + str.size())));
      }

      str = stream.str();
      return Buffer(str.data(), str.data() + str.size());
    }

    template <template <InterfaceMode> class Dispatcher>
    template <typename ReturnType>
    ReturnType Marshaller<Dispatcher>::DeserializeReturnValue(const Buffer& buffer)
    {
      IStream stream(std::string(buffer.begin(), buffer.end()));
      IArchive archive(stream);

      return Deserialize<ReturnType>(archive);
    }

    template <template <InterfaceMode> class Dispatcher>
    Detail::RemoteFunctionCall Marshaller<Dispatcher>::DeserializeFunctionDispatchHeader(const Buffer& data)
    {
      IStream stream(std::string(data.begin(), data.end()));
      IArchive archive(stream);

      return Deserialize<Detail::RemoteFunctionCall>(archive);
    }

    template <template <InterfaceMode> class Dispatcher>
    template <typename ReturnType, typename ArgumentTypes, typename Implementaion>
    Buffer Marshaller<Dispatcher>::DeserializeAndExecuteFunctionCall(const Buffer& paramData, Implementaion& implementation)
    {
      IStream istream(std::string(paramData.begin(), paramData.end()));
      IArchive iarchive(istream);

      OStream ostream;
      OArchive oarchive(ostream);

      FunctionCallHelper<ReturnType, ArgumentTypes>()(iarchive, oarchive, implementation);

      auto str = ostream.str();
      return Buffer(str.data(), str.data() + str.size());
    }

    template <template <InterfaceMode> class Dispatcher>
    template <typename ArgumentTypes, typename Argument, typename... RemainingArguments>
    void Marshaller<Dispatcher>::SerializeArguments(OArchive& archive, Argument&& argument, RemainingArguments&&... remainingArguments)
    {
      using ArgumentType = std::remove_reference_t<boost::mpl::front<ArgumentTypes>::type>;

      // check number of arguments (ArgumentTypes vs RemainingArguments)
      static_assert(boost::mpl::size<ArgumentTypes>::value == (sizeof...(remainingArguments)+1), "invalid numer of arguments supplied");

      // check that argument is convertible to first type in ArgumentTypes sequenze
      static_assert(std::is_convertible<Argument, ArgumentType>::value, "unable to convert supplied argument to expected argument type");

      // serialize argument
      Serialize<ArgumentType>(archive, argument);

      // serialize remaining arguments using recursive call OR terminate recursion by calling sentinal overload
      SerializeArguments<boost::mpl::pop_front<ArgumentTypes>::type>(archive, std::forward<RemainingArguments>(remainingArguments)...);
    }


    // sentinal overload to end recursion
    template <template <InterfaceMode> class Dispatcher>
    template <typename ArgumentTypes>
    void Marshaller<Dispatcher>::SerializeArguments(OArchive& /*archive*/)
    {
      static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "Not all arguments have been serialized, this overload must only be called as a recursion sentinal!");
    }

    template <template <InterfaceMode> class Dispatcher>
    using DefaultMarshaller = Marshaller<Dispatcher>;

  }  // namespace V1
}  // namespace CppRpc


// macro BOOST_CLASS_VERSION is a bit quirky, does not work inside namespaces nor with forward declarations
BOOST_CLASS_VERSION(CppRpc::V1::Version, CppRpc::V1::LibraryVersion)
BOOST_CLASS_VERSION(CppRpc::V1::Detail::RemoteFunctionCall, CppRpc::V1::LibraryVersion)
BOOST_CLASS_VERSION(CppRpc::V1::Detail::RemoteExceptionData, CppRpc::V1::LibraryVersion)

#endif
