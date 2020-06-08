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
    (timeout)
    (signal))

  QUINCE_MAP_CLASS(FunctionCall,
    (id)
    (memorystepping_id)
    (sequence)
    (function_no))

  QUINCE_MAP_CLASS(TestCaseCall,
    (id)
    (testcase_id)
    (functioncall_id)
    (success)
    (result_value))

  QUINCE_MAP_CLASS(FunctionArgs,
    (id)
    (functioncall_id)
    (args))

  QUINCE_MAP_CLASS(MemoryDiff,
    (id)
    (testcasecall_id)
    (index)
    (before)
    (after))

  QUINCE_MAP_CLASS(GlobalDiff,
    (id)
    (testcasecall_id)
    (index)
    (before)
    (after))

  struct Entities::Internal {
    quince_sqlite::database db;
    quince::serial_table<SeedSuite> seed_suites;
    quince::serial_table<Stepping> steppings;
    quince::serial_table<MemoryStepping> memory_steppings;
    quince::table<Implementation> implementations;
    quince::serial_table<TestCase> testcases;
    quince::serial_table<FunctionCall> function_calls;
    quince::serial_table<TestCaseCall> testcase_calls;
    quince::serial_table<MemoryDiff> memory_diffs;
    quince::serial_table<GlobalDiff> global_diffs;
    quince::serial_table<FunctionArgs> function_args;
    
    Internal(std::string const& filename, bool initialize_new_db) : 
        db{filename}, 
        seed_suites{db},
        steppings{db}, 
        memory_steppings{db},
        implementations{db}, 
        testcases{db},
        function_calls{db},
        testcase_calls{db},
        memory_diffs{db},
        global_diffs{db},
        function_args{db} { 
        
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

      function_calls.specify_foreign(function_calls->memorystepping_id, memory_steppings, memory_steppings->id);
      function_calls.open();

      testcase_calls.specify_foreign(testcase_calls->testcase_id, testcases, testcases->id);
      testcase_calls.specify_foreign(testcase_calls->functioncall_id, function_calls, function_calls->id);
      testcase_calls.open();

      memory_diffs.specify_foreign(memory_diffs->testcasecall_id, testcase_calls, testcase_calls->id);
      memory_diffs.open();

      global_diffs.specify_foreign(global_diffs->testcasecall_id, testcase_calls, testcase_calls->id);
      global_diffs.open();

      function_args.specify_foreign(function_args->functioncall_id, function_calls, function_calls->id);
      function_args.open();

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
  
  quince::serial Entities::StoreTestCase(quince::serial memorystepping_id, int implementation_id, int64_t timestamp, bool success, bool timeout, int signal) {
    return this->internal->testcases.insert(
      TestCase {
        {}, memorystepping_id, implementation_id, timestamp, success, timeout, signal
      }
    );
  }

  quince::serial Entities::StoreFunctionCall(FunctionCall obj) {
    return this->internal->function_calls.insert(obj);
  }

  quince::serial Entities::StoreTestCaseCall(TestCaseCall obj) {
    return this->internal->testcase_calls.insert(obj);
  }
  
  quince::serial Entities::StoreFunctionArgs(FunctionArgs obj) {
    return this->internal->function_args.insert(obj);
  }

  quince::serial Entities::StoreMemoryDiff(MemoryDiff obj) {
    return this->internal->memory_diffs.insert(obj);
  }

  quince::serial Entities::StoreGlobalDiff(GlobalDiff obj) {
    return this->internal->global_diffs.insert(obj);
  }
}