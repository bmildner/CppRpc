#include "cpprpc/Interface.h"

#include <string>
#include <functional>

struct TestImpl
{
  static void TestFunc1() {}
  static int  TestFunc2() { return 1; }
  static int  TestFunc3(int i) { return i; }
  static bool TestFunc4(const std::string& str) { return !str.empty(); }
  static bool TestFunc5(const std::string& str, bool enable) { return enable ? !str.empty() : false; }

  static const CppRpc::Name Name;
};

const CppRpc::Name TestImpl::Name = {"Test"};


template <typename Implementation, CppRpc::InterfaceMode Mode>
class Interface : public CppRpc::Interface<Mode>
{
  public:

    Interface(CppRpc::Transport<Mode>& transport)
    : CppRpc::Interface<Mode>(transport, Implementation::Name, {1, 1})
    {}

    // setup callable functions
    Interface::Function<void(void)>                     TestFunc1 = {*this, "TestFunc1", &Implementation::TestFunc1};
    Interface::Function<int(void)>                      TestFunc2 = {*this, "TestFunc2", std::function<int(void)>(&Implementation::TestFunc2)};  // test std::function object
    Interface::Function<int(int)>                       TestFunc3 = {*this, "TestFunc3", [] (int i) { return Implementation::TestFunc3(i); }};   // test lambda function
    Interface::Function<bool(const std::string&)>       TestFunc4 = {*this, "TestFunc4", &Implementation::TestFunc4};
    Interface::Function<bool(const std::string&, bool)> TestFunc5 = {*this, "TestFunc5", &Implementation::TestFunc5};

    //Interface::Function<std::function<void(void)>, Mode> TestFuncBad = {*this, "TestFunc1", &Implementation::TestFunc1};  // must not compile (T must be a function type, static assert)
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
  
  b = client.TestFunc5("foo", true);

  //client.TestFunc();  // must not compile (TestFunc not a member of TestClient)
  //b = client.TestFunc5(false);  // must not compile (invalid number of arguments, static assert)
  //b = client.TestFunc5(true, "foo");  // must not compile (unable to convert argument, static assert)  

  return 0;
}
