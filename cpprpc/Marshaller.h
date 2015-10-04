#ifndef CPPRPC_MARSHALLER_H
#define CPPRPC_MARSHALLER_H

#pragma once

#include <sstream>
#include <type_traits>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{
  inline namespace V1
  {

    template<class Archive>
    inline void serialize(Archive& ar, Version& ver, const unsigned int /*version*/)
    {
      ar & ver.m_Major;
      ar & ver.m_Minor;
    }

    namespace Detail
    {
      template<class Archive>
      inline void serialize(Archive& ar, FunctionDispatchHeader& funcDispHeader, const unsigned int /*version*/)
      {
        ar & funcDispHeader.m_LibraryVersion;
        ar & funcDispHeader.m_Interface;
        ar & funcDispHeader.m_Version;
        ar & funcDispHeader.m_Function;
      }
    }

    template <typename Dispatcher = DefaultDispatcher>
    class Marshaller
    {
        using Archive = boost::archive::text_oarchive;

      public:
        using OStream = std::ostringstream;
        using OArchive = boost::archive::text_oarchive;

        using IStream = std::istringstream;
        using IArchive = boost::archive::text_iarchive;

        template <typename ArgumentTypes, typename... Arguments>
        Buffer SerializeFunctionCall(const Interface& interface, const Name& functionName, Arguments&&... arguments) const
        {
          // check number of arguments (ArgumentTypes vs Arguments)
          static_assert(boost::mpl::size<ArgumentTypes>::value == sizeof...(arguments), "invalid number of arguments supplied");

          OStream stream;
          OArchive archive(stream);

          Serialize(archive, Dispatcher::FunctionDispatchHeader(interface, functionName));

          SerializeArguments<ArgumentTypes>(archive, std::forward<Arguments>(arguments)...);

          auto str = stream.str();
          return Buffer(str.data(), str.data() + str.size());
        }

        template <typename ReturnType>
        ReturnType DeserializeReturnValue(const Buffer& buffer) const
        {
          IStream stream(std::string(buffer.begin(), buffer.end()));
          IArchive archive(stream);

          return Deserialize<ReturnType>(archive);
        }

      private:
        
        template <typename ArgumentTypes, typename Argument, typename... RemainingArguments>
        void SerializeArguments(OArchive& archive, Argument&& argument, RemainingArguments&&... remainingArguments) const
        {
          // check number of arguments (ArgumentTypes vs RemainingArguments)
          static_assert(boost::mpl::size<ArgumentTypes>::value == (sizeof...(remainingArguments) + 1), "invalid numer of arguments supplied");

          // check that argument is convertible to first type in ArgumentTypes sequenze
          static_assert(std::is_convertible<Argument, std::remove_reference<boost::mpl::front<ArgumentTypes>::type>::type>::value, "unable to convert supplied argument to expected argument type");

          // serialize argument
          Serialize<boost::mpl::front<ArgumentTypes>::type>(archive, std::forward<Argument>(argument));

          // serialize remaining arguments using recursive call OR terminate recursion by calling sentinal overload
          SerializeArguments<boost::mpl::pop_front<ArgumentTypes>::type>(archive, std::forward<RemainingArguments>(remainingArguments)...);
        }

        // sentinal overload to end recursion
        template <typename ArgumentTypes>
        void SerializeArguments(OArchive& /*archive*/) const
        {
          // all arguments must have been serialized; validate usage as recursion sentinal
          static_assert(boost::mpl::size<ArgumentTypes>::value == 0, "");
        }

        template <typename T>
        void Serialize(OArchive& archive, T&& data) const
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

            return t;
          }
        };

        // spezialisation of DeserializeHelper for T = void
        template <>
        struct DeserializeHelper<void>
        {
          static void Deserialize(IArchive& /*archive*/)
          {}
        };

        template <typename T>
        T Deserialize(IArchive& archive) const
        {
          return DeserializeHelper<T>::Deserialize(archive);
        }

    };


    using DefaultMarshaller = Marshaller<>;

  }  // namespace V1
}  // namespace CppRpc

#endif
