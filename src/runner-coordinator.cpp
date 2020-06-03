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

std::tuple<pid_t, int> SpawnTester(std::string const& path) {
  pid_t pid;

  // Prepare pipe
  int fd[2];
  pipe(fd);

  // Split
  pid = fork();

  if(pid == 0) {
    // Child process
    close(fd[0]); // Close read
    dup2(fd[1], STDOUT_FILENO); // Copy to STDOUT
    dup2(fd[1], STDERR_FILENO); // Copy STDERR to STDOUT
    close(fd[1]); // Close existing file

    // Execute the runner
    execl(path.c_str(), path.c_str(),
                        "-mode", "single",
                        "-input", "/dev/shm/randomized-wasm",
                        "-memory", "/dev/shm/randomized-memory",
                        (char*)0);
    std::abort(); // Error
  } else { 
    close(fd[1]); // Close write
    return { pid, fd[0] }; // Return process id and pipe fileno
  }
}

void FuzzingLoop(CommandLineArgument& args) {
  CorePatternScope corePattern { args.dumpCore };

  

  if(!std::filesystem::exists((char const*)args.outputFolder))
    std::filesystem::create_directory((char const*)args.outputFolder);


  dfw::db::Entities entities { dfw::strjoin(args.outputFolder, "/fuzzer.db") };
  // Enable creating a core dump

  // Generate a new random seed
  uint64_t this_seed;
  if(args.reproduceSeed.set) {
    this_seed = args.reproduceSeed;
  } else {
    std::mt19937 re(std::time(NULL));
    this_seed = re();
  }

  std::string wasm_gen_cmd;
  std::string argfolder { args.programCommand };

  auto slash = std::find_if(argfolder.rbegin(), argfolder.rend(), [&] (char a) { return a == '/' ? true : false; });
  
  if(slash != argfolder.rend()) {
    argfolder = std::string { argfolder.begin(), slash.base() };
  }
  std::cout << "Argfolder: " << argfolder << std::endl;
  {
    // Build argument
    std::stringstream ss;
    ss << argfolder << "random-gen";
    ss << " -block-size " << args.randomSize;
    ss << " -seed " << this_seed;
    ss << " -output /dev/shm/randomized-wasm";
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
  __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, std::ios::out);
  std::ostream os(&filebuf);

  InstallSigaction();

  // Store seed ID in DB
  auto seed = entities.StoreSeedConfig(this_seed, args.randomSize);

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

    for(int i = 0; i < 10; i++) {
      std::cout << "memstep: " << i;
      // Inner loop increment the memory
      os << "m" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      os.flush();
      auto memstep = entities.StoreMemoryStepping(step, i);
      
      // Start two parallel process of V8 and SpiderMonkey
      std::string v8_args;
      {
        // Build argument
        std::stringstream ss;
        ss << argfolder << "runner-v8";
        //ss << " -mode single";
        //ss << " -input /dev/shm/randomized-wasm";
        //ss << " -memory /dev/shm/randomized-memory";
        //ss << " 2>&1";
        v8_args = ss.str();
      }
      
      std::string spidermonkey_args;
      {
        // Build argument
        std::stringstream ss;
        ss << argfolder << "runner-spidermonkey";
        //ss << " -mode single";
        //ss << " -input /dev/shm/randomized-wasm";
        //ss << " -memory /dev/shm/randomized-memory";
        //ss << " 2>&1";
        spidermonkey_args = ss.str();
      }
      
      std::cout << " * runner start * ";
      std::cout.flush();

      // Worker runner
      auto runner = [] (std::string args, std::string name) {
        //FILE* process = popen(args.c_str(), "r");
        auto [pid, pipeno] = SpawnTester(args);
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

          if(log.rfind("ERROR", 0) == 0) result = 1; // This is error
          
          return std::make_tuple(result == 0, std::move(log), timeout, signal);
        }
        
      };

      // Parallelize
      auto v8_task = std::async(std::launch::async, runner, v8_args, "v8");
      auto spidermonkey_task = std::async(std::launch::async, runner, spidermonkey_args, "spidermonkey");

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

      entities.StoreTestCase(memstep, (int)dfw::db::Entities::ID::V8, 
                             std::time(NULL), v8_success, std::move(v8_log), v8_timeout, v8_signal);

      entities.StoreTestCase(memstep, (int)dfw::db::Entities::ID::SpiderMonkey, 
                             std::time(NULL), spidermonkey_success, 
                             std::move(spidermonkey_log), spidermonkey_timeout, spidermonkey_signal);

      std::cout << std::endl;

      if(global_exit) {
        std::cout << "Exitting..." << std::endl;
        goto END;
      }
    }
  }

END:
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