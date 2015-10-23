#ifndef CPPRPC_MARSHALLER_H
#define CPPRPC_MARSHALLER_H

#pragma once

#include <sstream>
#include <type_traits>
#include <cassert>

#pragma warning(push)
#pragma warning(disable: 4100)  // boost/serialization/collections_load_imp.hpp(67): warning C4100: 'item_version': unreferenced formal parameter
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/vector.hpp>
#pragma warning(pop)

#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/pop_back.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"
#include "cpprpc/Exception.h"


BOOST_CLASS_VERSION(CppRpc::V1::Version, CppRpc::V1::LibraryVersion)
BOOST_CLASS_VERSION(CppRpc::V1::Detail::FunctionDispatchHeader, CppRpc::V1::LibraryVersion)


namespace CppRpc
{
  inline namespace V1
  {
    namespace Detail
    {

      template<class Archive>
      inline void serialize(Archive& ar, FunctionDispatchHeader& funcDispHeader, const unsigned int version)
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
          throw Detail::ExceptionImpl<LibraryVersionMissmatch>("Version of class FunctionDispatchHeader not equal to LibraryVersionV1");
        }
      }

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
        throw Detail::ExceptionImpl<LibraryVersionMissmatch>("Version of class Version not equal to LibraryVersionV1");
      }
    }


    template <template <InterfaceMode> class Dispatcher = DefaultDispatcher>
    class Marshaller
    {
      private:
        using OStream = std::ostringstream;
        using OArchive = boost::archive::text_oarchive;

        using IStream = std::istringstream;
        using IArchive = boost::archive::text_iarchive;

      public:

        template <typename ArgumentTypes, InterfaceMode Mode, typename... Arguments>
        static Buffer SerializeFunctionCall(const Interface<Mode>& interface, const Name& functionName, Arguments&&... arguments);

        template <typename ReturnType>
        static ReturnType DeserializeReturnValue(const Buffer& buffer);

        static Detail::FunctionDispatchHeader DeserializeFunctionDispatchHeader(const Buffer& data);

        template <typename ReturnType, typename ArgumentTypes, typename Implementaion>
        static Buffer DeserializeAndExecuteFunctionCall(const Buffer& paramData, Implementaion& implementation);

      private:

        template <typename ArgumentTypes, typename Argument, typename... RemainingArguments>
        static void SerializeArguments(OArchive& archive, Argument&& argument, RemainingArguments&&... remainingArguments);

        // sentinal overload to end recursion
        template <typename ArgumentTypes>
        static void SerializeArguments(OArchive& /*archive*/);


        template <typename ReturnType, typename ArgumentTypes, std::size_t ArgumentCount = boost::mpl::size<ArgumentTypes>::value>
        struct FunctionCallHelper
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value > 0, "");

          template <typename Implementaion, typename... Arguments>
          static void Do(IArchive& iarchive, OArchive& oarchive, Implementaion& implementation, Arguments&&... arguments)
          {
            using ParameterType = std::remove_reference_t<boost::mpl::front<ArgumentTypes>::type>;

            // deserialize parameter
            ParameterType param = Deserialize<std::remove_const_t<ParameterType>>(iarchive);

            // deserialize remaining parameters OR do function call
            FunctionCallHelper<ReturnType, boost::mpl::pop_front<ArgumentTypes>::type>::Do(iarchive, oarchive, implementation, std::forward<Arguments>(arguments)..., param);
          }
        };

        // spezialisation for ReturnType != void AND no more parameters
        template <typename ReturnType, typename ArgumentTypes>
        struct FunctionCallHelper<ReturnType, ArgumentTypes, 0>
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");
          static_assert(!std::is_void<ReturnType>::value, "");

          template <typename Implementaion, typename... Arguments>
          static void Do(IArchive& /*archive*/, OArchive& oarchive, Implementaion& implementation, Arguments&&... arguments)
          {
            // TODO: add exception handling !
            // call function implementation AND serialize result
            Serialize<ReturnType>(oarchive, implementation(std::forward<Arguments>(arguments)...));
          }
        };

        // spezialisation for ReturnType == void AND no more parameters
        template <typename ArgumentTypes>
        struct FunctionCallHelper<void, ArgumentTypes, 0>
        {
          static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");

          template <typename Implementaion, typename... Arguments>
          static void Do(IArchive& /*archive*/, OArchive& /*oarchive*/, Implementaion& implementation, Arguments&&... arguments)
          {
            // TODO: add exception handling !
            // call function implementation
            implementation(std::forward<Arguments>(arguments)...);
          }
        };


        template <typename T>
        static void Serialize(OArchive& archive, T&& data)
        {
          archive << data;
        }


        template <typename T>
        struct DeserializeHelper
        {
          static T Deserialize(IArchive& archive)
          {
            T t;

            archive >> t;

            return std::move(t);
          }
        };

        // spezialisation for T = void
        template <>
        struct DeserializeHelper<void>
        {
          static void Deserialize(IArchive& /*archive*/)
          {}
        };

        template <typename T>
        static T Deserialize(IArchive& archive)
        {
          return DeserializeHelper<T>::Deserialize(archive);
        }

    };

    template <template <InterfaceMode> class Dispatcher>
    template <typename ArgumentTypes, InterfaceMode Mode, typename... Arguments>
    Buffer Marshaller<Dispatcher>::SerializeFunctionCall(const Interface<Mode>& interface, const Name& functionName, Arguments&&... arguments)
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

        // serialize dispatch header
        Serialize(archive, Dispatcher<Mode>::FunctionDispatchHeader(interface, functionName, Buffer(str.data(), str.data() + str.size())));
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
    Detail::FunctionDispatchHeader Marshaller<Dispatcher>::DeserializeFunctionDispatchHeader(const Buffer& data)
    {
      IStream stream(std::string(data.begin(), data.end()));
      IArchive archive(stream);

      return Deserialize<Detail::FunctionDispatchHeader>(archive);
    }

    template <template <InterfaceMode> class Dispatcher>
    template <typename ReturnType, typename ArgumentTypes, typename Implementaion>
    Buffer Marshaller<Dispatcher>::DeserializeAndExecuteFunctionCall(const Buffer& paramData, Implementaion& implementation)
    {
      IStream istream(std::string(paramData.begin(), paramData.end()));
      IArchive iarchive(istream);

      OStream ostream;
      OArchive oarchive(ostream);

      FunctionCallHelper<ReturnType, ArgumentTypes>::Do(iarchive, oarchive, implementation);

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
      Serialize<ArgumentType>(archive, std::forward<Argument>(argument));

      // serialize remaining arguments using recursive call OR terminate recursion by calling sentinal overload
      SerializeArguments<boost::mpl::pop_front<ArgumentTypes>::type>(archive, std::forward<RemainingArguments>(remainingArguments)...);
    }


    // sentinal overload to end recursion
    template <template <InterfaceMode> class Dispatcher>
    template <typename ArgumentTypes>
    void Marshaller<Dispatcher>::SerializeArguments(OArchive& /*archive*/)
    {
      // all arguments must have been serialized; validate usage as recursion sentinal
      static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");
    }
    using DefaultMarshaller = Marshaller<>;

  }  // namespace V1
}  // namespace CppRpc

#endif
