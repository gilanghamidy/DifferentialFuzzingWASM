#include "runner-common.h"
#include "fuzzer-db.h"

#include <fstream>
#include <random>
#include <sstream>
#include <ext/stdio_filebuf.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <future>
#include <string>
#include <mutex>
#include <filesystem>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

char const* CorePatternToDump = "core.%e.%s.%p.%t.dmp";
char const* CorePatternFile = "/proc/sys/kernel/core_pattern";

struct CommandLineArgument {
  dfw::CommandLineArg<uint64_t> randomSize { "-block-size", true };
  dfw::CommandLineArg<char const*> outputFolder { "-output-folder", true };

  dfw::CommandLineArg<bool> reproduce { "-reproduce-seed" };
  dfw::CommandLineArg<uint64_t> reproduceStep { "-step", false, 0 };
  dfw::CommandLineArg<uint64_t> reproduceSeed { "-seed", false, 0 };
  dfw::CommandLineArg<uint64_t> reproduceMemoryStep { "-memory-step", false, 0 };

  dfw::CommandLineArg<bool> dumpCore { "-dump-core" };

  char const* programCommand;

  CommandLineArgument(int argc, char const* argv[]) : programCommand(argv[0]) {
    dfw::CommandLineConsumer { argc, argv, 
                          std::ref(randomSize),
                          std::ref(outputFolder),
                          std::ref(reproduceSeed)};
  }
};

bool volatile global_exit = false;

void ExitHandler (int sig, siginfo_t *siginfo, void *context) {
  global_exit = true;
  printf("Exit Called\n");
  fflush(stdout);
}

class CorePatternScope {
  bool use;
  std::string previous_core_pattern;
public:
  CorePatternScope(bool use) : use(use) {
    std::string previous_core_pattern { };
    {
      std::ifstream input_file { CorePatternFile, std::ios_base::in };
      std::getline(input_file, this->previous_core_pattern);
    }

    if(use) {
      std::cout << "Replacing core pattern temporarily. Calling sudo" << std::endl;
      
      std::string command { "echo \"" };
      command.append(CorePatternToDump);
      command.append("\" | sudo tee ");
      command.append(CorePatternFile);
      auto res = system(command.c_str());

      std::cout << "Core pattern replaced: " << (res == 0) << std::endl;
    }
  }

  ~CorePatternScope() {
    if(use) {
      std::string command { "echo \"" };
      command.append(previous_core_pattern);
      command.append("\" | sudo tee ");
      command.append(CorePatternFile);

      auto res = system(command.c_str());

      std::cout << "Core pattern restored: " << (res == 0) << std::endl;
    }
  }
};

class POpenScope {
  FILE* f;
public:
  POpenScope(FILE* f) : f(f) { }
  ~POpenScope() {
    std::cout << "Closing process " << (int64_t) f << std::endl;
    pclose(f);
  }
};

class FilenoScope {
  int fileno;
public:
  FilenoScope(int fileno) : fileno(fileno) { }
  ~FilenoScope() {
    close(fileno);
  }
};

class ProcessScope {
  int pid;
public:
  ProcessScope(int pid) : pid(pid) { }
  ~ProcessScope() {
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
  }
};




void InstallSigaction() {
  struct sigaction act;
	memset (&act, '\0', sizeof(act));
 
	/* Use the sa_sigaction field because the handles has two additional parameters */
	act.sa_sigaction = &ExitHandler;
 
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	if (sigaction(SIGINT, &act, NULL) < 0) {
		printf("Error in sigaction");
		std::abort();
	}
}

std::tuple<pid_t, int> SpawnTester(std::string const& path, 
                                   std::string const& input_wasm,
                                   std::string const& mem_path,
                                   std::string const& arg_seed) {
  pid_t pid;

  // Prepare pipe
  int fd[2];
  pipe(fd);

  // Split
  pid = fork();

  if(pid == 0) {
    // Child process
    close(fd[0]); // Close read
    dup2(fd[1], COMMON_FILE_DESCRIPTOR); // Copy to STDOUT
    int stdnull = open("/dev/null", O_RDONLY);
    dup2(stdnull, STDOUT_FILENO);
    dup2(stdnull, STDERR_FILENO); // Copy STDERR to STDOUT
    close(fd[1]); // Close existing file
    close(stdnull);
    // Execute the runner
    execl(path.c_str(), path.c_str(),
                        "-mode", "single",
                        "-input", input_wasm.c_str(),
                        "-memory", mem_path.c_str(),
                        "-arg-seed", arg_seed.c_str(),
                        (char*)0);
    std::abort(); // Error
  } else { 
    close(fd[1]); // Close write
    return { pid, fd[0] }; // Return process id and pipe fileno
  }
}

std::tuple<pid_t, int, int> SpawnGenerator(
          std::string const& argfolder, 
          uint64_t seed, 
          uint64_t block_size) {
  pid_t pid;

  // Prepare pipe
  int fd[2];
  pipe(fd);

  // Split
  pid = fork();

  if(pid == 0) {
    // Child process
    
    dup2(fd[0], STDIN_FILENO); // Copy to STDIN
    dup2(fd[1], STDOUT_FILENO); // Copy to STDOUT
    dup2(fd[1], STDERR_FILENO); // Copy STDERR to STDOUT
    close(fd[1]); // Close existing file
    close(fd[0]); // Close existing file

     // Build argument
    std::string seed_str = std::to_string(seed),
                block_size_str = std::to_string(block_size),
                path = argfolder + "random-gen";

    // Execute the runner
    execl(path.c_str(), path.c_str(),
                        "-block-size", block_size_str.c_str(),
                        "-seed", seed_str.c_str(),
                        "-output", "/dev/shm/randomized-wasm",
                        "-memory", "/dev/shm/randomized-memory",
                        (char*)0);
    std::abort(); // Error
  } else { 
    return { pid, fd[1], fd[0] }; // Return process id and pipe fileno
  }
}

bool GetLineOrEnd(std::stringstream& str, std::string& out) {
  std::getline(str, out);
  if(out == "ENDCOMPARE") return true;
  else return false;
}



void CompareLogs(std::string& v8_log, 
                 std::string& moz_log,
                 dfw::db::Entities& entities,
                 quince::serial memstep,
                 quince::serial v8_id,
                 quince::serial moz_id) {

  using namespace rapidjson;

  if(v8_log.length() == 0 || moz_log.length() == 0) { 
    if (v8_log.length() == 0)
      std::cout << "v8 empty log ";
    if (moz_log.length() == 0)
      std::cout << "moz empty log ";
    return; 
  }

  // Fix the crash cases
  if(v8_log.length() > 0 && *v8_log.rbegin() != ']') v8_log += ']';  
  if(moz_log.length() > 0 && *moz_log.rbegin() != ']') moz_log += ']';
  
  Document v8_log_doc;
  v8_log_doc.Parse(v8_log.c_str());

  Document moz_log_doc;
  moz_log_doc.Parse(moz_log.c_str());

  std::cout << "Processing Log:" << std::endl;

  //std::cout << v8_log << std::endl << std::endl;
  //std::cout << moz_log << std::endl;

  // Starts with an array
  auto v8_log_iter = v8_log_doc.Begin();
  auto moz_log_iter = moz_log_doc.Begin();

  int64_t sequence = 0;

  // When everything is still the same
  while(v8_log_iter != v8_log_doc.End() && moz_log_iter != moz_log_doc.End()) {
    
    // Get the object
    decltype(auto) v8_exec = v8_log_iter->GetObject();
    decltype(auto) moz_exec = moz_log_iter->GetObject();

    // Continue
    ++v8_log_iter;
    ++moz_log_iter;

    std::string func_name = v8_exec["FunctionName"].GetString();

    assert(std::strcmp(v8_exec["FunctionName"].GetString(), 
                       moz_exec["FunctionName"].GetString()) == 0);

    auto func_num = std::strtol(&func_name[4], nullptr, 10);

    auto v8_elapsed = std::strtol(v8_exec["Elapsed"].GetString(), nullptr, 10); 
    auto moz_elapsed = std::strtol(moz_exec["Elapsed"].GetString(), nullptr, 10);

    auto v8_success = v8_exec["Success"].GetBool();
    auto moz_success = moz_exec["Success"].GetBool();

    auto functioncall_id = entities.StoreFunctionCall(dfw::db::FunctionCall {
                             {}, memstep, sequence, func_num
                           });
    
    // Store the args
    for(auto& arg : v8_exec["Args"].GetArray()) {
      auto argval = std::strtol(arg.GetString(), nullptr, 10);
      entities.StoreFunctionArgs(dfw::db::FunctionArgs { {}, functioncall_id, argval });
    }

    boost::optional<int64_t> v8_result, moz_result;
    if(v8_success && v8_exec.HasMember("Result")) { 
      v8_result = std::strtol(v8_exec["Result"].GetString(), nullptr, 10); 
    }

    if(moz_success && moz_exec.HasMember("Result")) { 
      moz_result = std::strtol(moz_exec["Result"].GetString(), nullptr, 10); 
    }

    // Store call each test case
    auto v8_case = entities.StoreTestCaseCall(dfw::db::TestCaseCall { {}, v8_id, functioncall_id, 
                                              v8_success, v8_elapsed, v8_result });
    
    auto moz_case = entities.StoreTestCaseCall(dfw::db::TestCaseCall { {}, moz_id, functioncall_id, 
                                               moz_success, moz_elapsed, moz_result });

    // Check if there is memory diff
    if(v8_exec.HasMember("MemoryDiff")) {
      for(auto& member : v8_exec["MemoryDiff"].GetObject()) {
        auto index = std::strtol(member.name.GetString(), nullptr, 10);
        auto old_val = member.value[0].GetInt64();
        auto new_val = member.value[1].GetInt64();

        entities.StoreMemoryDiff(dfw::db::MemoryDiff {
          {}, v8_case, index, old_val, new_val
        });
      }
    }
    if(moz_exec.HasMember("MemoryDiff")) {
      for(auto& member : moz_exec["MemoryDiff"].GetObject()) {
        auto index = std::strtol(member.name.GetString(), nullptr, 10);
        auto old_val = member.value[0].GetInt64();
        auto new_val = member.value[1].GetInt64();

        entities.StoreMemoryDiff(dfw::db::MemoryDiff {
          {}, moz_case, index, old_val, new_val
        });
      }
    }

    // Check if there is a global diff
    if(v8_exec.HasMember("GlobalDiff")) {
      for(auto& member : v8_exec["GlobalDiff"].GetObject()) {
        auto index = std::strtol(member.name.GetString(), nullptr, 10);
        auto old_val = (int64_t)std::strtoull(member.value[0].GetString(), nullptr, 10);
        auto new_val = (int64_t)std::strtoull(member.value[1].GetString(), nullptr, 10);

        entities.StoreGlobalDiff(dfw::db::GlobalDiff {
          {}, moz_case, index, old_val, new_val
        });
      }
    }
    if(moz_exec.HasMember("GlobalDiff")) {
      for(auto& member : moz_exec["GlobalDiff"].GetObject()) {
        auto index = std::strtol(member.name.GetString(), nullptr, 10);
        auto old_val = (int64_t)std::strtoull(member.value[0].GetString(), nullptr, 10);
        auto new_val = (int64_t)std::strtoull(member.value[1].GetString(), nullptr, 10);

        entities.StoreGlobalDiff(dfw::db::GlobalDiff {
          {}, moz_case, index, old_val, new_val
        });
      }
    }
    sequence++;
    std::cout << "o";
  }
}

char const* memories[] = { "memory/zero.mem",
                           "memory/one.mem", 
                           "memory/rand1.mem", 
                           "memory/rand2.mem", 
                           "memory/rand3.mem" };

void FuzzingLoop(CommandLineArgument& args) {
  CorePatternScope corePattern { args.dumpCore };

  if(!std::filesystem::exists((char const*)args.outputFolder))
    std::filesystem::create_directory((char const*)args.outputFolder);

  dfw::db::Entities entities { dfw::strjoin(args.outputFolder, "/fuzzer.db") };
  // Enable creating a core dump

  // Generate a new random seed
  int64_t this_seed;
  std::mt19937 re(std::time(NULL));
  this_seed = re();

  if(args.reproduceSeed.set) {
    this_seed = args.reproduceSeed;
  }
  // Reseed
  re.seed(this_seed);

  std::string wasm_gen_cmd;
  std::string argfolder { args.programCommand };

  auto slash = std::find_if(argfolder.rbegin(), argfolder.rend(), [&] (char a) { return a == '/' ? true : false; });
  
  if(slash != argfolder.rend()) {
    argfolder = std::string { argfolder.begin(), slash.base() };
  }

  
  std::cout << "Argfolder: " << argfolder << std::endl;
  std::string input_wasm = "/dev/shm/randomized-wasm-";
  input_wasm += std::to_string(this_seed);
  {
    // Build argument
    std::stringstream ss;
    ss << argfolder << "random-gen";
    ss << " -block-size " << args.randomSize;
    ss << " -seed " << this_seed;
    ss << " -output " << input_wasm;
    ss << " -memory /dev/shm/randomized-memory";
    ss << " > /dev/null";
    wasm_gen_cmd = ss.str();
  }

  // Open process to generate WASM and Memory
  FILE* wasm_gen_p = popen(wasm_gen_cmd.c_str(), "w");
  POpenScope wasm_gen_scope(wasm_gen_p);

  if(wasm_gen_p == NULL) {
    std::cout << "ERROR OPENING GENERATOR: "
              << wasm_gen_cmd << std::endl;
    std::abort();
  }

  int posix_handle = fileno(wasm_gen_p);
  __gnu_cxx::stdio_filebuf<char> filebuf_out(posix_handle, std::ios::out);
  std::ostream os(&filebuf_out);

  InstallSigaction();

  // Store seed ID in DB
  auto seed = entities.StoreSeedConfig(this_seed, args.randomSize);
  entities.Flush();

  std::cout << "seed: " << this_seed << "\n";
  for(int i = 0; i < 5000; i++) {
    std::cout << "step: " << i << "\n";
    // Loop increment the stepping
    os << "w" << std::endl;
    os.flush();

    // Sleep for a moment
    // Need to find a way to get notification back from popen
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto step = entities.StoreStepping(seed, i);

    for(int i = 0; i < 5; i++) {
      std::cout << "memstep: " << i;
      // Inner loop increment the memory
      
      // Start two parallel process of V8 and SpiderMonkey
      std::string v8_args;
      {
        // Build argument
        std::stringstream ss;
        ss << argfolder << "runner-v8";
        v8_args = ss.str();
      }
      
      std::string spidermonkey_args;
      {
        // Build argument
        std::stringstream ss;
        ss << argfolder << "runner-spidermonkey";
        spidermonkey_args = ss.str();
      }

      std::string mem_args;
      {
        // Build argument
        std::stringstream ss;
        ss << argfolder << memories[i];
        mem_args = ss.str();
      }
      
      std::cout << " * runner start * ";
      std::cout.flush();

      // Worker runner
      auto runner = [] (std::string args, std::string input_wasm, std::string mem_args, std::string arg_seed, std::string name) {
        //FILE* process = popen(args.c_str(), "r");
        auto [pid, pipeno] = SpawnTester(args, input_wasm, mem_args, arg_seed);
        FilenoScope pipeno_scope(pipeno);

        //int posix_handle = fileno(process);
        __gnu_cxx::stdio_filebuf<char> filebuf(pipeno, std::ios::in);
        std::istream is(&filebuf);

        std::atomic_bool terminate_signal(false);

        // Split again inside an async task
        using TaskFunc = std::tuple<std::string, bool>(void);
        std::packaged_task<TaskFunc> read_task ([&is, &name, &terminate_signal] {
          char buffer[4096]; // Eat the buffer until EOF

          std::memset(buffer, 0, sizeof(buffer));
          std::stringstream ss;
          std::string line;

          bool timeout = false;
          
          while (!is.eof()) {
            if(terminate_signal.load()) {
              std::cout << " * receive timeout signal * ";
              ss << "PROCESS TIMEOUT\n";
              timeout = true;
              break;
            }
              
            auto read = is.readsome(buffer, sizeof(buffer));
            if(read != 0) {
              ss.write(buffer, read);
            } else {
              is.peek(); // Trigger read to EOF
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
          }
          
          std::cout << " * finished processing " << name << " * ";
          return std::make_tuple(ss.str(), timeout);
        });

        auto read_future = read_task.get_future();
        std::thread t(std::move(read_task));
        t.detach();
        auto future_state = read_future.wait_for(std::chrono::seconds(10));

        if(future_state != std::future_status::ready) {
          // Timeout, force close
          std::cout << " * process timeout, closing... * ";
          terminate_signal.store(true);
          std::cout.flush();

          // Kill child process
          kill(pid, SIGKILL);
          waitpid(pid, NULL, 0);
          auto [log, timeout] = read_future.get();
          return std::make_tuple(false, std::move(log), true, 0);

        } else {
          // Check child process return status
          int status;
          int result;
          waitpid(pid, &status, 0);
          auto [log, timeout] = read_future.get();
          int signal = 0;
          if(WIFEXITED(status)) {
            result = WEXITSTATUS(status);
          } else if(WIFSIGNALED(status)) {
            signal = WTERMSIG(status);
          }

          //if(log.rfind("ERROR", 0) == 0) result = 1; // This is error
          
          return std::make_tuple(result == 0, std::move(log), timeout, signal);
        }
        
      };

      int64_t arg_seed = re();

      // Parallelize
      auto v8_task = std::async(std::launch::async, runner, v8_args, input_wasm, mem_args, std::to_string(arg_seed), "v8");
      auto spidermonkey_task = std::async(std::launch::async, runner, spidermonkey_args, input_wasm, mem_args, std::to_string(arg_seed), "spidermonkey");

      auto [v8_success, 
            v8_log, 
            v8_timeout, 
            v8_signal] = v8_task.get();

      
      auto [spidermonkey_success, 
            spidermonkey_log, 
            spidermonkey_timeout, 
            spidermonkey_signal] = spidermonkey_task.get();
      
      if(!v8_success) {
        std::cout << " v8 failed";
      }
      
      if(!spidermonkey_success) {
        std::cout << " spidermonkey failed";
      }

      auto memstep = entities.StoreMemoryStepping(step, i, arg_seed);
      
      auto v8_id = entities.StoreTestCase(memstep, (int)dfw::db::Entities::ID::V8, 
                                          std::time(NULL), v8_success, v8_timeout, v8_signal);

      auto sm_id = entities.StoreTestCase(memstep, (int)dfw::db::Entities::ID::SpiderMonkey, 
                                          std::time(NULL), spidermonkey_success, spidermonkey_timeout, spidermonkey_signal);
      
      CompareLogs(v8_log, spidermonkey_log, entities, memstep, v8_id, sm_id);

      std::cout << std::endl;

      if(global_exit) {
        std::cout << "Exitting..." << std::endl;
        goto END;
      }

      if(i % 100 == 0 && i != 0)
        entities.Flush(); // Write every 100 records
    }
  }

END:
  entities.Flush();
  os << "q" << std::endl;
  os.flush();
  return;
}



void Reproduce(CommandLineArgument& args) {

}

int main(int argc, char const* argv[]) {

  CommandLineArgument args { argc, argv };
  
  if(!args.reproduce)
    FuzzingLoop(args);
  else {
    Reproduce(args);
  }
  

  return 0;
}