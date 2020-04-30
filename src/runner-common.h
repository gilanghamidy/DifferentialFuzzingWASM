#ifndef RUNNER_COMMON_H
#define RUNNER_COMMON_H

#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>
#include <errno.h>
#include <string.h>
#include <cstring>
#include <list>
#include <initializer_list>
#include <iostream>
#include <sstream>

namespace dfw {
std::vector<uint8_t> OpenInput(char const *fileName);
void DumpDisassemble(std::ostream &output,
                     std::vector<uint8_t> const &instructions);
void WriteOutput(char const *fileName, std::vector<uint8_t> const &buf);
std::vector<uint8_t> GenerateRandomData(uint64_t seed, uint32_t len);

typedef std::optional<std::vector<uint8_t>>
FuncNameToBinFunctor(std::string const &);
void DumperLooper(std::function<FuncNameToBinFunctor> transform);

template <typename T> bool ConsumeArg(char const *arg, T &target);

template <> inline bool ConsumeArg(char const *arg, char const *&target) {
  target = arg;
  return true;
}

template <> inline bool ConsumeArg(char const *arg, uint64_t &target) {
  target = std::stoll(std::string{arg});
  return true;
}

template <typename T> struct CommandLineArg {
  T value{};
  bool set{false};
  static constexpr bool canConsume = true;
  char const *flag;
  bool required;

  CommandLineArg(char const *flag, bool require = false, T default_val = {})
      : flag(flag), required(require), value(default_val) {}

  bool Match(char const *arg) { return std::strcmp(arg, flag) == 0; }

  bool Consume(char const *arg) {
    set = true;
    return ConsumeArg<T>(arg, value);
  }

  operator T() { return value; }
};

template <> struct CommandLineArg<bool> {
  bool value{false};
  static constexpr bool canConsume = false;
  char const *flag;
  bool required;

  CommandLineArg(char const *flag) : flag(flag) {}

  bool Match(char const *arg) {
    bool res = std::strcmp(arg, flag) == 0;
    if (res) {
      value = true;
    }

    return res;
  }

  operator bool() { return value; }
};

struct CommandLineConsumer {
  template <typename T, typename... TNext>
  void Consume(std::list<char const *> &argsStr, T arg, TNext... argsVar) {
    if (arg.get().Match(argsStr.front())) {
      argsStr.pop_front();
      if constexpr (std::decay_t<decltype(
                        std::declval<T>().get())>::canConsume) {
        try {
          arg.get().Consume(argsStr.front());
        } catch (std::exception const &ex) {
          std::cerr << "Error parsing the argument: " << arg.get().flag
                    << " expects: " << typeid(decltype(arg.get().value)).name()
                    << std::endl;
          exit(-1);
        }
        argsStr.pop_front();
      }
    } else {
      Consume(argsStr, argsVar...);
    }
  }

  void Consume(std::list<char const *> argsStr) {
    throw std::logic_error("Unknown argument");
  }
  template <typename T> void ValidateRequireImpl(T arg) {
    if (arg.get().required && !arg.get().set) {
      std::cerr << "Required argument is not set: " << arg.get().flag
                << std::endl;
      exit(-1);
    }
  }

  template <typename T, typename... TArgs>
  void ValidateRequire(T arg, TArgs... args) {
    ValidateRequireImpl(arg);
    ValidateRequire(args...);
  } 

  template <typename T> void ValidateRequire(T arg) {
    ValidateRequireImpl(arg);
  }

  template <typename... T>
  CommandLineConsumer(int argc, char const *argv[], T... argsVar) {
    std::list<char const *> items;
    std::copy(&argv[1], &argv[argc], std::inserter(items, items.begin()));

    while (!items.empty())
      Consume(items, argsVar...);

    ValidateRequire(argsVar...);
  }
};

struct FuzzerRunnerCLArgs {
  dfw::CommandLineArg<char const*> input { "-input", true };
  dfw::CommandLineArg<char const*> mode { "-mode", false, "interactive" };
  dfw::CommandLineArg<char const*> memory { "-memory", false };
  dfw::CommandLineArg<int> count { "-invoke-count", false, 50 };
  char const* exec_path;
  FuzzerRunnerCLArgs(int argc, char const* argv[]) : exec_path(argv[0]) {
    dfw::CommandLineConsumer { argc, argv, 
                               std::ref(input),
                               std::ref(mode),
                               std::ref(memory) };
  }
};

enum class WasmType {
  Void,
  I32,
  I64,
  F32,
  F64
};

char const* WasmTypeToString(WasmType t);

struct FunctionInfo {
  std::string function_name;
  WasmType return_type;
  std::vector<WasmType> parameters;
};

struct JSValue {
  WasmType type;
  union {
    uint32_t i32;
    uint64_t i64;
    float    f32;
    double   f64;
  };
};

struct DataRange {
  std::vector<uint8_t> const& data;
  std::vector<uint8_t>::const_iterator pos;

  DataRange(std::vector<uint8_t> const& data) : 
    data(data), pos(data.begin()) { }
  
  template<typename T>
  T get() {
    size_t need = sizeof(T);
    auto end = pos + need;
    if(end >= data.end()) {
      pos = data.begin();
      end = pos + need;
    }

    auto raw = &*pos;
    T ret {};
    std::memcpy(&ret, raw, sizeof(T));
    pos += need;
    return ret;
  }
};

template<>
inline bool DataRange::get<bool>() {
  return get<uint8_t>() % 2 == 0;
}

#define ERROR_IF_FALSE(x, msg) if (!(x)) { printf((msg)); printf("\n"); return -1; }
#define RETURN_IF_FALSE(x) if (!(x)) return false


void PrintJSValue(JSValue const& v);

template<typename T>
class FuzzerRunner {
  T runner;
public:
  template<typename... Args>
  FuzzerRunner(Args&&... a) : runner(std::forward<Args>(a)...) { }
  
  void Looper() {
    while(true) {
      std::string input;
      std::cout << ">>> ";

      // Split args
      std::cin >> input;

      // quit
      if(input == "quit" || input == "q")
        break;
      else if(input == "dump") {
        // Get function name from next token
        std::cin >> input;
        if(auto func_dump = runner.DumpFunction(input); !func_dump) {
          std::cout << "Unknown function: " << input << std::endl;
        } else {
          std::cout << "Dumping function: " << input << std::endl;
          dfw::DumpDisassemble(std::cout, func_dump.value());
        }
      } else if(input == "list") {
        for(dfw::FunctionInfo const& info : runner.Functions()) {
          std::cout << "func \"" << info.function_name << "\" (param ";
          bool first = true;
          for(auto t : info.parameters) {
            if(first) first = false; else std::cout << ", ";
            std::cout << WasmTypeToString(t);
          }
          std::cout << ") (return " << WasmTypeToString(info.return_type) << ")";
          std::cout << std::endl;
        }
      } else if(input == "invoke") {
        std::cin >> input;
        decltype(auto) functions = runner.Functions();
        auto selected_func = std::find_if(functions.begin(), functions.end(),
                                          [&] (dfw::FunctionInfo const& info) { 
                                            return info.function_name == input;
                                          });
        if(selected_func == functions.end()) {
          std::cout << "Unknown function: " << input << std::endl;
        } else {
          // Get the remaining string
          std::getline(std::cin, input);

          std::istringstream arg_str(input);

          std::vector<JSValue> args;
          JSValue val;
          int cnt = 0;
          for(WasmType param : selected_func->parameters) {
            if(arg_str.eof()) 
              break;
            cnt += 1;
            val.type = param;
            try {
              switch(param) {
                case WasmType::I32: {
                  arg_str >> val.i32;
                  args.emplace_back(val);
                  break;
                }
                case WasmType::I64: {
                  arg_str >> val.i64;
                  args.emplace_back(val);
                  break;
                }
                case WasmType::F32: {
                  arg_str >> val.f32;
                  args.emplace_back(val);
                  break;
                }
                case WasmType::F64: {
                  arg_str >> val.f64;
                  args.emplace_back(val);
                  break;
                }
                default: break;
              }
            } catch(std::exception const& ex) {
              std::cout << "Wrong argument #" << cnt << ": expected: " << WasmTypeToString(val.type) << std::endl;
              break;
            }
          }

          if(args.size() != selected_func->parameters.size()) {
            std::cout << "Wrong argument count: " << args.size() << ", expected: " << selected_func->parameters.size() << std::endl;
            continue;
          }
          
          std::cout << "Invoke function: " << input << std::endl;
          std::optional<dfw::JSValue> res = runner.InvokeFunction(selected_func->function_name, args);
          
          if(res != std::nullopt) {
            PrintJSValue(res.value());
          } else {
            std::cout << "Error when invoking function" << std::endl;
          }
          
          continue;
        }

      } else if(input == "instantiate") {
        if(!runner.InitializeExecution())
          std::cout << "Cannot instantiate WASM module." << std::endl;
      } else if(input == "memimportgen") {
        std::vector<uint32_t> mem_input;
        int a = 0;
        std::generate_n(std::inserter(mem_input, mem_input.end()), 16 * 64 * 1024 / 4, [&]() { return a++; });

        auto res = runner.MarshallMemoryImport((uint8_t*)mem_input.data(), mem_input.size() * 4);
        if(res == false)
          std::cout << "Failed marshalling memory\n";
      } else if(input == "memimport") {
        std::cin >> input; // memory file
        LoadMemory(input.c_str());
        std::cout << "Memory imported." << std::endl;
      } else {
        std::cout << "Unknown command: " << input << std::endl;
      }      
      // Flush the remainder
      std::getline(std::cin, input);
    }
  }

  auto LoadMemory(char const* memfile) {
    auto mem_input = OpenInput(memfile);
    runner.MarshallMemoryImport(mem_input.data(), mem_input.size());
    return mem_input;
  }

  std::vector<dfw::JSValue> GenerateArgs(std::vector<WasmType> const& param_types, DataRange& random) {
    std::vector<dfw::JSValue> args;
    JSValue arg;
    for(auto param_type : param_types) {
      arg.type = param_type;
      switch (param_type)
      {
      case WasmType::I32:
        arg.i32 = random.get<uint32_t>();
        break;
      case WasmType::I64:
        arg.i64 = random.get<uint64_t>();
        break;
      case WasmType::F32:
        arg.f32 = random.get<float>();
        break;
      case WasmType::F64:
        arg.f64 = random.get<double>();
        break;
      default:
        std::cerr << "Unexpected param_type when generating arguments\n";
        std::abort();
      }
      args.push_back(arg);
    }
    return args;
  }

  bool SingleRun(dfw::FuzzerRunnerCLArgs const& args) {
    std::optional<std::vector<uint8_t>> memory;
    if(args.memory.set) {
      memory.emplace(LoadMemory(args.memory.value));
    }

    RETURN_IF_FALSE(runner.InitializeExecution());

    // Loop function call, cover all possible functions
    decltype(auto) functions = runner.Functions();
    DataRange random { *memory };
    decltype(auto) funcs = runner.Functions();
    size_t func_count = funcs.size();

    for(int i = 0; i < args.count.value; ++i) {
      size_t select_func = random.get<uint16_t>() % func_count;
      
      std::cout << "Invoke: " << funcs[select_func].function_name << " ";
      std::optional<dfw::JSValue> res = 
          runner.InvokeFunction(funcs[select_func].function_name, 
                                GenerateArgs(funcs[select_func].parameters, random));
      
      if(res.has_value()) {
        std::cout << " success: "; 
        PrintJSValue(*res);
      } else {
        std::cout << " failed";
      }

      std::cout << std::endl;
    }
    

    return true;
  }
  
  int Run(int argc, char const* argv[]) {
    dfw::FuzzerRunnerCLArgs args { argc, argv };

    ERROR_IF_FALSE(runner.InitializeModule(args), "Failed initializing and compiling WASM");
    
    if(std::strcmp(args.mode, "interactive") == 0) {
      Looper();
    } else if(std::strcmp(args.mode, "single") == 0) {
      ERROR_IF_FALSE(SingleRun(args), "Failed executing test case.");
    }
    return 0;
  }
};

#undef ERROR_IF_FALSE

} // namespace dfw

#endif