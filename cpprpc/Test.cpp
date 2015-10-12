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


template <typename Implementation, CppRpc::InterfaceMode mode>
class Interface : public CppRpc::Interface
{
  public:

    Interface()
    : CppRpc::Interface(Implementation::Name, {1, 1})
    {}

    // setup callable functions
    CppRpc::Function<void(void),                     mode> TestFunc1 = {*this, "TestFunc1", &Implementation::TestFunc1Impl};
    CppRpc::Function<int(void),                      mode> TestFunc2 = {*this, "TestFunc2", std::function<int(void)>(&Implementation::TestFunc2Impl)};  // test std::function object
    CppRpc::Function<int(int),                       mode> TestFunc3 = {*this, "TestFunc3", [] (int i) { return Implementation::TestFunc3Impl(i); }};   // test lambda function
    CppRpc::Function<bool(const std::string&),       mode> TestFunc4 = {*this, "TestFunc4", &Implementation::TestFunc4Impl};
    CppRpc::Function<bool(const std::string&, bool), mode> TestFunc5 = {*this, "TestFunc5", &Implementation::TestFunc5Impl};

};


template <CppRpc::InterfaceMode mode>
using TestInterface = Interface<Test, mode>;

using TestServer = TestInterface<CppRpc::InterfaceMode::Server>;
using TestClient = TestInterface<CppRpc::InterfaceMode::Client>;


int main()
{
  TestServer server;

  int i;
  bool b;

  //server.TestFunc1();
  //i = server.TestFunc2();
  //i = server.TestFunc3(4711);
  //b = server.TestFunc4("Hallo");
  //b = server.TestFunc5("foo", true);


  TestClient client;

  client.TestFunc1();

  i = client.TestFunc2();

  i = client.TestFunc3(4711);

  b = client.TestFunc4("Hallo");
  
  b = client.TestFunc5("foo", true);

  //client.TestFunc();  // causes compilation error (TestFunc not a member of TestClient)
  //b = client.TestFunc5(false);  // causes compilation error (invalid number of arguments, static assert)
  //b = client.TestFunc5(true, "foo");  // causes compilation error (unable to convert argument, static assert)  

  return 0;
}
