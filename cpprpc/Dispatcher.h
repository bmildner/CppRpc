#ifndef CPPRPC_DISPATCHER_H
#define CPPRPC_DISPATCHER_H

#pragma once

#include <string>

#include <boost/serialization/access.hpp>

#include "cpprpc/Types.h"

namespace CppRpc
{
  inline namespace V1
  {
    class Interface;

    namespace Detail
    {
      class FunctionDispatchHeader
      {
        public:
          FunctionDispatchHeader(const Interface& interface, const Name& functionName);

        private:
          friend class boost::serialization::access;

          template<class Archive>
          void serialize(Archive & ar, const unsigned int /*version*/)
          {
            ar & Interface;
            ar & Version;
            ar & Function;
          }

          Name Interface;
          Version Version;
          Name Function;
      };
    }

    class Dispatcher
    {
      public:
        using FunctionDispatchHeader = Detail::FunctionDispatchHeader;

    };

    using DefaultDispatcher = Dispatcher;

  }  // namespace V1
}  // namespace CppRpc

#endif

