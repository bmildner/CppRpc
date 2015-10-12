#include "Types.h"

#include <boost/format.hpp>

namespace CppRpc
{
  inline namespace V1
  {

    std::string Version::str() const
    {
      return (boost::format("%1%.%2%") % m_Major % m_Minor).str();
    }

  }  // namespace V1
}  // namespace CppRpc
