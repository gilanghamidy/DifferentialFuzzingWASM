#include <string>
#include <string_view>
#include <quince/serial.h>
#include <memory>

namespace dfw::db {
  struct SeedSuite {
    quince::serial id;
    int64_t seed;
    int64_t block_size;
    std::string fuzzer_commit;

    static constexpr std::string_view table_name { "seed_suites" };
    static constexpr auto primary_key { &SeedSuite::id };
  };

  struct Stepping {
    quince::serial id;
    quince::serial seedsuite_id;
    int64_t step;

    static constexpr std::string_view table_name { "steppings" };
    static constexpr auto primary_key { &Stepping::id };
  };

  struct MemoryStepping {
    quince::serial id;
    quince::serial stepping_id;
    int64_t step;
    int64_t arg_seed;

    static constexpr std::string_view table_name { "memory_steppings" };
    static constexpr auto primary_key { &MemoryStepping::id };
  };

  struct Implementation {
    int id;
    std::string implementation;

    static constexpr std::string_view table_name { "implementations" };
    static constexpr auto primary_key { &Implementation::id };
  };

  struct TestCase {
    quince::serial id;
    quince::serial memorystepping_id;
    int implementation_id;
    int64_t timestamp;
    bool success;
    bool timeout;
    int signal;

    static constexpr std::string_view table_name { "testcases" };
    static constexpr auto primary_key { &TestCase::id };
  };

  struct FunctionCall {
    quince::serial id;
    quince::serial memorystepping_id;
    int64_t sequence;
    int64_t function_no;

    static constexpr std::string_view table_name { "function_call" };
    static constexpr auto primary_key { &FunctionCall::id };
  };

  struct FunctionArgs {
    quince::serial id;
    quince::serial functioncall_id;
    int64_t args;

    static constexpr std::string_view table_name { "function_args" };
    static constexpr auto primary_key { &FunctionArgs::id };
  };

  struct TestCaseCall {
    quince::serial id;
    quince::serial testcase_id;
    quince::serial functioncall_id;

    bool success;
    int64_t elapsed;
    boost::optional<int64_t> result_value;

    static constexpr std::string_view table_name { "testcase_call" };
    static constexpr auto primary_key { &TestCaseCall::id };
  };

  struct MemoryDiff {
    quince::serial id;
    quince::serial testcasecall_id;
    int64_t index;
    int64_t before;
    int64_t after;

    static constexpr std::string_view table_name { "memorydiff_call" };
    static constexpr auto primary_key { &MemoryDiff::id };
  };

  struct GlobalDiff {
    quince::serial id;
    quince::serial testcasecall_id;
    int64_t index;
    int64_t before;
    int64_t after;

    static constexpr std::string_view table_name { "globaldiff_call" };
    static constexpr auto primary_key { &GlobalDiff::id };
  };

  class Entities {
    struct Internal;

    std::unique_ptr<Internal> internal;

  public:
    enum class ID : uint8_t {
      V8 = 1,
      SpiderMonkey = 2
    };

    Entities(std::string const& filename);
    ~Entities();

    quince::serial StoreSeedConfig(int64_t seed, int64_t blocksize);
    quince::serial StoreStepping(quince::serial seed_id, int64_t step);
    quince::serial StoreMemoryStepping(quince::serial stepping_id, int64_t step, int64_t arg_seed);
    quince::serial StoreTestCase(quince::serial memorystepping_id, int implementation_id, int64_t timestamp, bool success, bool timeout, int signal);
    quince::serial StoreFunctionCall(FunctionCall obj);
    quince::serial StoreTestCaseCall(TestCaseCall obj);
    quince::serial StoreFunctionArgs(FunctionArgs obj);
    quince::serial StoreMemoryDiff(MemoryDiff obj);
    quince::serial StoreGlobalDiff(GlobalDiff obj);
  };
}