#ifndef CPPRPC_MARSHALLER_H
#define CPPRPC_MARSHALLER_H

#pragma once

#include <sstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/pop_front.hpp>

#include "cpprpc/Types.h"
#include "cpprpc/Dispatcher.h"

namespace CppRpc
{
  inline namespace V1
  {

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
          static_assert(boost::mpl::size<ArgumentTypes>::value == sizeof...(arguments), "invalid numer of arguments supplied");

          OStream stream;
          OArchive archive(stream);

          Serialize(archive, Dispatcher::FunctionDispatchHeader(interface, functionName));

          SerializeParam(archive, std::forward<Arguments>(arguments)...);

          auto str = stream.str();
          return Buffer(str.data(), str.data() + str.size());
        }

        template <typename ReturnType>
        ReturnType DeserializeReturnValue(const Buffer& buffer)
        {
          IStream stream(std::string(buffer.begin(), buffer.end()));
          IArchive archive(stream);

          return Deserialize<ReturnType>(archive);
        }

      private:
        
        template <typename First, typename... RemainingArguments>
        void SerializeParam(OArchive& archive, First&& first, RemainingArguments&&... remainingArguments) const
        {
          Serialize(archive, std::forward<First>(first));
          SerializeParam(archive, std::forward<RemainingArguments>(remainingArguments)...);
        }

        void SerializeParam(OArchive& /*buffer*/) const
        {}

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

        template <>
        struct DeserializeHelper<void>
        {
          static void Deserialize(IArchive& /*archive*/)
          {}
        };

        template <typename T>
        T Deserialize(IArchive& archive)
        {
          return DeserializeHelper<T>::Deserialize(archive);
        }

    };


    using DefaultMarshaller = Marshaller<>;

  }  // namespace V1
}  // namespace CppRpc

#endif
