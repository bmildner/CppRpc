#include "cpprpc/Interface.h"

#include <string>
#include <functional>
#include <map>
#include <list>
#include <stdexcept>

#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/format.hpp>


struct TestImplementation
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

const CppRpc::Name TestImplementation::Name = {"TestImplementation"};

TestImplementation::TestFunc6ReturnType TestImplementation::TestFunc6(const TestFunc6ParamType& input)
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


struct TestImplementation_Throws
{
  template <std::size_t Number>
  struct Exception : std::runtime_error
  {
    Exception()
    : std::runtime_error((boost::format("TestFunc%1%") % Number).str())
    {}

    virtual ~Exception() noexcept override = default;
  };

  using TestFunc1Exception = Exception<1>;
  using TestFunc2Exception = Exception<2>;
  using TestFunc3Exception = Exception<3>;
  using TestFunc4Exception = Exception<4>;
  using TestFunc5Exception = Exception<5>;
  using TestFunc6Exception = Exception<6>;

  static void TestFunc1() { throw TestFunc1Exception(); }
  static int  TestFunc2() { throw TestFunc2Exception(); }
  static int  TestFunc3(int /*i*/) { throw TestFunc3Exception(); }
  static bool TestFunc4(const std::string& /*str*/) { throw TestFunc4Exception(); }
  static bool TestFunc5(const std::string& /*str*/, bool /*enable*/) { throw TestFunc5Exception(); }


  template <typename Key1, typename Key2, typename Value>
  using MapOfMaps = std::map<Key1, std::map<Key2, Value>>;

  template <typename Key, typename Value>
  using MapOfLists = std::map<Key, std::list<Value>>;

  using TestFunc6ReturnType = MapOfMaps<int, std::size_t, std::string>;
  using TestFunc6ParamType = MapOfLists<int, std::string>;

  static TestFunc6ReturnType TestFunc6(const TestFunc6ParamType& /*input*/) { throw TestFunc6Exception(); }

  static const CppRpc::Name Name;
};

const CppRpc::Name TestImplementation_Throws::Name = {"TestImplementation_Throws"};



template <typename Implementation, CppRpc::InterfaceMode Mode>
class Interface : public CppRpc::Interface<Mode>
{
  public:
    
    template <typename Argument>
    Interface(Argument&& argument)
    : CppRpc::Interface<Mode>(std::forward<Argument>(argument), Implementation::Name, {1, 1})
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
using TestInterface = Interface<TestImplementation, Mode>;

using TestServer = TestInterface<CppRpc::InterfaceMode::Server>;
using TestClient = TestInterface<CppRpc::InterfaceMode::Client>;


template <CppRpc::InterfaceMode Mode>
using TestInterfaceThrows = Interface<TestImplementation_Throws, Mode>;

using TestServerThrows = TestInterfaceThrows<CppRpc::InterfaceMode::Server>;
using TestClientThrows = TestInterfaceThrows<CppRpc::InterfaceMode::Client>;


int main()
{
  CppRpc::V1::LocalDummyTransport transport;

  TestServer::DispatcherHandle serverDispatcher = CppRpc::V1::MakeDispatcherHandle(transport.GetServerTransport());

  TestServer server(serverDispatcher);

  TestClient client(transport.GetClientTransport());

  int i;
  bool b;

  client.TestFunc1();

  i = client.TestFunc2();

  i = client.TestFunc3(4711);

  b = client.TestFunc4("Hallo");
  
  b = true;
  b = client.TestFunc5("foo", b);

  TestImplementation::TestFunc6ParamType fkt6Param;

  fkt6Param[4711] = {"Hallo", "World!"};
  fkt6Param[815] = {"this", "is", "almost", "magic"};

  TestImplementation::TestFunc6ReturnType fkt6Ret = client.TestFunc6(fkt6Param);

  //client.TestFunc();  // must not compile (TestFunc not a member of TestClient)
  //b = client.TestFunc5(false);  // must not compile (invalid number of arguments, static assert)
  //b = client.TestFunc5(true, "foo");  // must not compile (unable to convert argument, static assert)  



  // test ecxeption handling

  TestServerThrows throwingServer(serverDispatcher);

  TestClientThrows throwingClient(client.GetDispatcher());

  // TODO: add test code that chacks for exception thrown and the exception type
  //throwingClient.TestFunc1(); 

  return 0;
}
