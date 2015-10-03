#include "cpprpc/Interface.h"

#include <string>

using namespace CppRpc;

template <InterfaceMode mode>
class TestInterface : public Interface
{
  public:

    TestInterface()
    : Interface("TestInterface", {1, 1})
    {}

    Function<void(void),                     mode> TestFunc1 = {*this, "TestFunc1", &TestFunc1Impl};
    Function<int(void),                      mode> TestFunc2 = {*this, "TestFunc2", &TestFunc2Impl};
    Function<int(int),                       mode> TestFunc3 = {*this, "TestFunc3", &TestFunc3Impl};
    Function<bool(const std::string&),       mode> TestFunc4 = {*this, "TestFunc4", &TestFunc4Impl};
    Function<bool(const std::string&, bool), mode> TestFunc5 = {*this, "TestFunc5", &TestFunc5Impl};

  private:
    static void TestFunc1Impl() {}
    static int TestFunc2Impl() { return 1; }
    static int TestFunc3Impl(int i) { return i; }
    static bool TestFunc4Impl(const std::string& str) { return !str.empty(); }
    static bool TestFunc5Impl(const std::string& str, bool enable) { return enable ? !str.empty() : false; }

};


int main()
{
  TestInterface<InterfaceMode::Server> server;

  int i;
  bool b;

  server.TestFunc1();

  i = server.TestFunc2();

  i = server.TestFunc3(4711);

  b = server.TestFunc4("Hallo");

  b = server.TestFunc5("foo", true);


  TestInterface<InterfaceMode::Client> client;

  client.TestFunc1();

  i = client.TestFunc2();

  i = client.TestFunc3(4711);

  b = client.TestFunc4("Hallo");
  
  b = client.TestFunc5(true, "foo");

  return 0;
}