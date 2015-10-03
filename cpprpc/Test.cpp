#include "cpprpc/Interface.h"

#include <string>


template <CppRpc::InterfaceMode mode>
class TestInterface : public CppRpc::Interface
{
  public:

    TestInterface()
    : Interface("TestInterface", {1, 1})
    {}

    CppRpc::Function<void(void),                     mode> TestFunc1 = {*this, "TestFunc1", &TestFunc1Impl};
    CppRpc::Function<int(void),                      mode> TestFunc2 = {*this, "TestFunc2", &TestFunc2Impl};
    CppRpc::Function<int(int),                       mode> TestFunc3 = {*this, "TestFunc3", &TestFunc3Impl};
    CppRpc::Function<bool(const std::string&),       mode> TestFunc4 = {*this, "TestFunc4", &TestFunc4Impl};
    CppRpc::Function<bool(const std::string&, bool), mode> TestFunc5 = {*this, "TestFunc5", &TestFunc5Impl};

  private:
    static void TestFunc1Impl()                                    {}
    static int  TestFunc2Impl()                                    { return 1; }
    static int  TestFunc3Impl(int i)                               { return i; }
    static bool TestFunc4Impl(const std::string& str)              { return !str.empty(); }
    static bool TestFunc5Impl(const std::string& str, bool enable) { return enable ? !str.empty() : false; }

};


int main()
{
  TestInterface<CppRpc::InterfaceMode::Server> server;

  int i;
  bool b;

  //server.TestFunc1();
  //i = server.TestFunc2();
  //i = server.TestFunc3(4711);
  //b = server.TestFunc4("Hallo");
  //b = server.TestFunc5("foo", true);


  TestInterface<CppRpc::InterfaceMode::Client> client;

  client.TestFunc1();

  i = client.TestFunc2();

  i = client.TestFunc3(4711);

  b = client.TestFunc4("Hallo");
  
  b = client.TestFunc5("foo", true);

  //b = client.TestFunc5(true, "foo");  // cuases compilation error (incompatible argument types, in Marshaller::SerializeArguments)
  //b = client.TestFunc5(false);  // cuases compilation error (invalid number of arguments, static assert)

  return 0;
}
