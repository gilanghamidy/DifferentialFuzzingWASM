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
    std::string result;
    bool timeout;
    int signal;

    static constexpr std::string_view table_name { "testcases" };
    static constexpr auto primary_key { &TestCase::id };
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
    quince::serial StoreMemoryStepping(quince::serial stepping_id, int64_t step);
    quince::serial StoreTestCase(quince::serial memorystepping_id, int implementation_id, int64_t timestamp, bool success, std::string&& result, bool timeout, int signal);
  };
}