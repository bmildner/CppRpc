#include "cpprpc/Interface.h"

#include <string>
#include <functional>

struct Test
{
  static void TestFunc1Impl() {}
  static int  TestFunc2Impl() { return 1; }
  static int  TestFunc3Impl(int i) { return i; }
  static bool TestFunc4Impl(const std::string& str) { return !str.empty(); }
  static bool TestFunc5Impl(const std::string& str, bool enable) { return enable ? !str.empty() : false; }

  static const CppRpc::Name Name;
};

const CppRpc::Name Test::Name = {"Test"};


template <typename Implementation, CppRpc::InterfaceMode Mode>
class Interface : public CppRpc::Interface<Mode>
{
  public:

    Interface(CppRpc::Transport<Mode>& transport)
    : CppRpc::Interface<Mode>(transport, Implementation::Name, {1, 1})
    {}

    // setup callable functions
    CppRpc::Function<void(void),                     Mode> TestFunc1 = {*this, "TestFunc1", &Implementation::TestFunc1Impl};
    CppRpc::Function<int(void),                      Mode> TestFunc2 = {*this, "TestFunc2", std::function<int(void)>(&Implementation::TestFunc2Impl)};  // test std::function object
    CppRpc::Function<int(int),                       Mode> TestFunc3 = {*this, "TestFunc3", [] (int i) { return Implementation::TestFunc3Impl(i); }};   // test lambda function
    CppRpc::Function<bool(const std::string&),       Mode> TestFunc4 = {*this, "TestFunc4", &Implementation::TestFunc4Impl};
    CppRpc::Function<bool(const std::string&, bool), Mode> TestFunc5 = {*this, "TestFunc5", &Implementation::TestFunc5Impl};

    //CppRpc::Function<std::function<void(void)>, Mode> TestFuncBad = {*this, "TestFunc1", &Implementation::TestFunc1Impl};  // must not compile (T must be a function type, static assert)
};


template <CppRpc::InterfaceMode Mode>
using TestInterface = Interface<Test, Mode>;

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
