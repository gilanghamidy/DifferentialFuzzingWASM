#include "fuzzer-db.h"
#include "quince/quince.h"
#include "quince_sqlite/database.h"

#include <filesystem>

namespace dfw::db {
  QUINCE_MAP_CLASS(SeedSuite, 
    (id)
    (seed)
    (block_size)
    (fuzzer_commit))

  QUINCE_MAP_CLASS(Stepping, 
    (id)
    (seedsuite_id)
    (step))

  QUINCE_MAP_CLASS(MemoryStepping, 
    (id)
    (stepping_id)
    (step))

  QUINCE_MAP_CLASS(Implementation, 
    (id)
    (implementation))


  QUINCE_MAP_CLASS(TestCase, 
    (id)
    (memorystepping_id)
    (implementation_id)
    (timestamp)
    (success)
    (result)
    (timeout)
    (signal))

  struct Entities::Internal {
    quince_sqlite::database db;
    quince::serial_table<SeedSuite> seed_suites;
    quince::serial_table<Stepping> steppings;
    quince::serial_table<MemoryStepping> memory_steppings;
    quince::table<Implementation> implementations;
    quince::serial_table<TestCase> testcases;
    
    Internal(std::string const& filename, bool initialize_new_db) : 
        db{ filename }, 
        seed_suites{ db },
        steppings{ db }, 
        memory_steppings{ db },
        implementations{ db }, 
        testcases{ db } { 
        
      // Open tables
      seed_suites.open();

      steppings.specify_foreign(steppings->seedsuite_id, seed_suites, seed_suites->id);
      steppings.open();

      memory_steppings.specify_foreign(memory_steppings->stepping_id, steppings, steppings->id);
      memory_steppings.open();

      implementations.open();

      testcases.specify_foreign(testcases->memorystepping_id, memory_steppings, memory_steppings->id);
      testcases.specify_foreign(testcases->implementation_id, implementations, implementations->id);
      testcases.open();

      if(initialize_new_db) {
        InitNewDb();
      }
    }

    void InitNewDb() {
      // Insert implementation list
      Implementation v8 { (int)ID::V8, "V8" },
                     sm { (int)ID::SpiderMonkey, "SpiderMonkey" };
      this->implementations.insert(v8);
      this->implementations.insert(sm);
    }
  };

  Entities::Entities(std::string const& filename) {
    bool file_exists = std::filesystem::exists(filename);
    internal = std::make_unique<Internal>(filename, !file_exists);
  }

  Entities::~Entities() {

  }

  quince::serial Entities::StoreSeedConfig(int64_t seed, int64_t blocksize) {
    return this->internal->seed_suites.insert(SeedSuite { {}, seed, blocksize, FUZZER_COMMIT });
  }
  
  quince::serial Entities::StoreStepping(quince::serial seed_id, int64_t step) {
    return this->internal->steppings.insert(Stepping { {}, seed_id, step });
  }
  
  quince::serial Entities::StoreMemoryStepping(quince::serial stepping_id, int64_t step) {
    return this->internal->memory_steppings.insert(MemoryStepping { {}, stepping_id, step });
  }
  
  quince::serial Entities::StoreTestCase(quince::serial memorystepping_id, int implementation_id, int64_t timestamp, bool success, std::string&& result, bool timeout, int signal) {
    return this->internal->testcases.insert(
      TestCase {
        {}, memorystepping_id, implementation_id, timestamp, success, std::move(result), timeout, signal
      }
    );
  }
  
}