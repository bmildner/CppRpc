#include "cpprpc/Interface.h"

#include <string>
#include <functional>
#include <map>
#include <list>

#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>


struct TestImpl
{
  static void TestFunc1() {}
  static int  TestFunc2() { return 1; }
  static int  TestFunc3(int i) { return i; }
  static bool TestFunc4(const std::string& str) { return !str.empty(); }
  static bool TestFunc5(const std::string& str, bool enable) { return enable ? !str.empty() : false; }


  template <typename Key1, typename Key2, typename Value>
  using MapOfMaps = std::map<Key1, std::map<Key2, Value>>;

  template <typename Key, typename Value>
  using MapOfLists = std::map<Key, std::list<Value>>;

  using TestFunc6ReturnType = MapOfMaps<int, std::size_t, std::string>;
  using TestFunc6ParamType = MapOfLists<int, std::string>;

  static TestFunc6ReturnType TestFunc6(const TestFunc6ParamType& input);

  static const CppRpc::Name Name;
};

const CppRpc::Name TestImpl::Name = {"Test"};


TestImpl::TestFunc6ReturnType TestImpl::TestFunc6(const TestFunc6ParamType& input)
{
  TestFunc6ReturnType result;

  for (const auto& list : input)
  {
    for (const auto& string : list.second)
    {
      result[list.first][string.size()] = string;
    }
  }

  return result;
}

template <typename Implementation, CppRpc::InterfaceMode Mode>
class Interface : public CppRpc::Interface<Mode>
{
  public:

    Interface(CppRpc::Transport<Mode>& transport)
    : CppRpc::Interface<Mode>(transport, Implementation::Name, {1, 1})
    {}

    // setup callable functions
    Function<void(void)>                     TestFunc1 = {*this, "TestFunc1", &Implementation::TestFunc1};
    Function<int(void)>                      TestFunc2 = {*this, "TestFunc2", std::function<int(void)>(&Implementation::TestFunc2)};  // test std::function object
    Function<int(int)>                       TestFunc3 = {*this, "TestFunc3", [] (int i) { return Implementation::TestFunc3(i); }};   // test lambda function
    Function<bool(const std::string&)>       TestFunc4 = {*this, "TestFunc4", &Implementation::TestFunc4};
    Function<bool(const std::string&, bool)> TestFunc5 = {*this, "TestFunc5", &Implementation::TestFunc5};

    Function<typename Implementation::TestFunc6ReturnType(typename const Implementation::TestFunc6ParamType&)> TestFunc6 = {*this, "TestFunc6", &Implementation::TestFunc6};
    

    //Function<std::function<void(void)>> TestFuncBad = {*this, "TestFunc1", &Implementation::TestFunc1};  // must not compile (T must be a function type, static assert)
};


template <CppRpc::InterfaceMode Mode>
using TestInterface = Interface<TestImpl, Mode>;

using TestServer = TestInterface<CppRpc::InterfaceMode::Server>;
using TestClient = TestInterface<CppRpc::InterfaceMode::Client>;


int main()
{
  CppRpc::LocalDummyTransport transport;

  TestServer server(transport.GetServerTransport());

  TestClient client(transport.GetClientTransport());

  int i;
  bool b;

  client.TestFunc1();

  i = client.TestFunc2();

  i = client.TestFunc3(4711);

  b = client.TestFunc4("Hallo");
  
  b = true;
  b = client.TestFunc5("foo", b);

  TestImpl::TestFunc6ParamType fkt6Param;

  fkt6Param[4711] = {"Hallo", "World!"};
  fkt6Param[815] = {"this", "is", "almost", "magic"};

  TestImpl::TestFunc6ReturnType fkt6Ret = client.TestFunc6(fkt6Param);

  //client.TestFunc();  // must not compile (TestFunc not a member of TestClient)
  //b = client.TestFunc5(false);  // must not compile (invalid number of arguments, static assert)
  //b = client.TestFunc5(true, "foo");  // must not compile (unable to convert argument, static assert)  

  return 0;
}
