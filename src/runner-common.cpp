#include "runner-common.h"

#include <random>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ext/stdio_filebuf.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <map>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>

#define ERROR_IF_FALSE(x, msg) if (!(x)) { printf((msg)); printf("\n"); return -1; }
#define RETURN_IF_FALSE(x) if (!(x)) return false

std::vector<uint8_t> dfw::OpenInput(char const* fileName) {
  std::ifstream inputFile(fileName);
  inputFile.seekg(0, std::ios::end);
  size_t length = inputFile.tellg();
  inputFile.seekg(0, std::ios::beg);
  std::vector<uint8_t> buf;
  buf.resize(length);
  inputFile.read((char*)buf.data(), length);
  return buf;
}

void dfw::PrintJSValue(JSValue const& v) {
  switch(v.type) {
    case WasmType::I32: {
      std::cout << "i32: " << v.i32 << std::endl;
      break;
    }
    case WasmType::I64: {
      std::cout << "i64: " << v.i64 << std::endl;
      break;
    }
    case WasmType::F32: {
      std::cout << "f32: " << v.f32 << std::endl;
      break;
    }
    case WasmType::F64: {
      std::cout << "f64: " << v.f64 << std::endl;
      break;
    }
    default: {
      std::cout << "void" << std::endl;
      break;
    }
  }
}

std::string StringJSValue(dfw::JSValue const& v) {
  using namespace dfw;
  std::stringstream ss;
  switch(v.type) {
    case WasmType::I32: {
      ss << "i32: " << v.i32;
      break;
    }
    case WasmType::I64: {
      ss << "i64: " << v.i64;
      break;
    }
    case WasmType::F32: {
      ss << "f32: " << v.f32;
      break;
    }
    case WasmType::F64: {
      ss << "f64: " << v.f64;
      break;
    }
    default: {
      ss << "void";
      break;
    }
  }
  return ss.str();
}

char const* dfw::WasmTypeToString(WasmType t) {
  switch (t)
  {
  case WasmType::I32: return "i32";
  case WasmType::I64: return "i64";
  case WasmType::F32: return "f32";
  case WasmType::F64: return "f64";
  default:            return "";
  }
}

void dfw::DumpDisassemble(std::ostream& output, std::vector<uint8_t> const& instructions) {
  // Create temporary file
  std::ofstream tempOut("/tmp/instruct.bin");
  tempOut.write((char*)instructions.data(), instructions.size());
  tempOut.flush();
  tempOut.close();

  

  // Call objdump
  // Najong hack to get istream lol
  std::string objdumpCmd { "objdump -D -Mintel,x86-64 -b binary -m i386 /tmp/instruct.bin" };
  int posix_handle = fileno(::popen(objdumpCmd.c_str(), "r"));
  __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, std::ios::in);
  std::istream is(&filebuf);
  
  // Print objdump output
  for(int i = 0; !is.eof();) {
    std::string line;
    std::getline(is, line);
    i++;
    if(i > 7) // Skip first 7 line
      output << line << "\n";
  }
}

void dfw::WriteOutput(char const* fileName, std::vector<uint8_t> const& buf) {
  std::ofstream outputFile(fileName);
  outputFile.write((const char*)buf.data(), buf.size());
  outputFile.flush();
  outputFile.close();
}

std::vector<uint8_t> dfw::GenerateRandomData(uint64_t seed, uint32_t len) {
  std::mt19937 re(seed);
  std::vector<uint8_t> ret;
  ret.resize(len);
  std::for_each(ret.begin(), ret.end(), [&] (uint8_t& val) { val = re(); });
  return ret;
}

void dfw::DumperLooper(std::function<dfw::FuncNameToBinFunctor> transform) {
  while(true) {
    std::string input;

    std::cout << ">>> ";
    std::getline(std::cin, input);

    if(input == "\\q")
      break;
    
     //(*res)[input];
    std::cout << "Dumping function: " << input << std::endl;
    
    if(auto func = transform(input); !func) 
      std::cout << "Unknown function!\n";
    else 
      dfw::DumpDisassemble(std::cout, func.value());
  }
}

void dfw::FuzzerRunnerBase::Looper() {
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
      if(auto func_dump = DumpFunction(input); !func_dump) {
        std::cout << "Unknown function: " << input << std::endl;
      } else {
        std::cout << "Dumping function: " << input << std::endl;
        dfw::DumpDisassemble(std::cout, func_dump.value());
      }
    } else if(input == "list") {
      for(dfw::FunctionInfo const& info : Functions()) {
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
      decltype(auto) functions = Functions();
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
        auto [res, elapsed] = InvokeFunction(selected_func->function_name, args);
        
        if(res != std::nullopt) {
          PrintJSValue(res.value());
        } else {
          std::cout << "Error when invoking function" << std::endl;
        }
        
        continue;
      }

    } else if(input == "instantiate") {
      if(!InitializeExecution())
        std::cout << "Cannot instantiate WASM module." << std::endl;
    } else if(input == "memimportgen") {
      std::vector<uint32_t> mem_input;
      int a = 0;
      std::generate_n(std::inserter(mem_input, mem_input.end()), 16 * 64 * 1024 / 4, [&]() { return a++; });

      auto res = MarshallMemoryImport((uint8_t*)mem_input.data(), mem_input.size() * 4);
      if(res == false)
        std::cout << "Failed marshalling memory\n";
    } else if(input == "memimport") {
      std::cin >> input; // memory file
      LoadMemory(input.c_str());
      std::cout << "Memory imported." << std::endl;
    } else if(input == "listglobal") {
      for(dfw::GlobalInfo const& info : Globals()) {
        std::cout << info.global_name << ": ";
        std::cout << WasmTypeToString(info.type);
        std::cout << std::endl;
      }
    } else if (input == "setglobal") { 
      std::cin >> input;
      decltype(auto) globals = Globals();
      auto selected_global = std::find_if(globals.begin(), globals.end(),
                                        [&] (dfw::GlobalInfo const& info) { 
                                          return info.global_name == input;
                                        });
      if(selected_global == globals.end()) {
        std::cout << "Unknown global: " << input << std::endl;
      } else {
        // Get the remaining string
        std::getline(std::cin, input);

        std::istringstream arg_str(input);
        JSValue val;
        val.type = selected_global->type;
        try {
          switch(selected_global->type) {
            case WasmType::I32: {
              arg_str >> val.i32;
              break;
            }
            case WasmType::I64: {
              arg_str >> val.i64;
              break;
            }
            case WasmType::F32: {
              arg_str >> val.f32;
              break;
            }
            case WasmType::F64: {
              arg_str >> val.f64;
              break;
            }
            default: break;
          }
          std::cout << "Setting global...\n";
          this->SetGlobal(selected_global->global_name, val);
          continue;
        } catch(std::exception const& ex) {
          std::cout << "Wrong global value expected: " << WasmTypeToString(val.type) << std::endl;
        }
      }
    } else if (input == "getglobal") { 
      std::cin >> input;
      PrintJSValue(this->GetGlobal(input));
    } else {
      std::cout << "Unknown command: " << input << std::endl;
    }      
    // Flush the remainder
    std::getline(std::cin, input);
  }
}

std::vector<uint8_t> dfw::FuzzerRunnerBase::LoadMemory(char const* memfile) {
  auto mem_input = dfw::OpenInput(memfile);
  MarshallMemoryImport(mem_input.data(), mem_input.size());
  return mem_input;
}

dfw::JSValue GetRandomValue(dfw::WasmType type, dfw::DataRange& random) {
  using namespace dfw;
  JSValue ret;
  ret.type = type;
  switch(type) {
    case WasmType::I32: {
      ret.i32 = random.get<int32_t>();
      break;
    }
    case WasmType::I64: {
      ret.i64 = random.get<int64_t>();
      break;
    }
    case WasmType::F32: {
      ret.f32 = random.get<float>();
      break;
    }
    case WasmType::F64: {
      ret.f64 = random.get<double>();
      break;
    }
    default: {
      break;
    }
  }
  return ret;
}

bool dfw::FuzzerRunnerBase::SingleRun(dfw::FuzzerRunnerCLArgs const& args) {
  using namespace rapidjson;

  auto flag = fcntl(COMMON_FILE_DESCRIPTOR, F_GETFD);

  std::ostream* output = &std::cout;
  __gnu_cxx::stdio_filebuf<char> filebuf_out(COMMON_FILE_DESCRIPTOR, std::ios::out);
  std::ostream os(&filebuf_out);

  if(flag >= 0) {
    output = &os;
  }
  
  std::optional<std::vector<uint8_t>> memory;
  //std::cout << "Load memory" << std::endl;
  if(args.memory.set) {
    memory.emplace(LoadMemory(args.memory.value));
  }
  //std::cout << "Initialize execution" << std::endl;
  RETURN_IF_FALSE(InitializeExecution());

  // Loop function call, cover all possible functions
  //std::cout << "Get functions" << std::endl;
  DataRange random { *memory };
  auto funcs = Functions();
  size_t func_count = funcs.size();
  //std::cout << "Start Looping\n" << std::endl;
  std::sort(funcs.begin(), funcs.end(), 
            [] (dfw::FunctionInfo const& a, dfw::FunctionInfo const& b) {
              return a.function_name > b.function_name;
            });

  *output << "[";

  // Initialize Global Values
  auto globals = Globals();
  std::map<std::string, JSValue> global_state;
  for(auto& global : globals) {
    auto init_val = GetRandomValue(global.type, random);
    SetGlobal(global.global_name, init_val);
    global_state.emplace(global.global_name, init_val);
  }

  for(int i = 0; i < args.count.value; ++i) {
    if(i != 0) *output << ",";

    // Prepare JSON Logging
    Document report;
    auto& reportArr = report.SetObject();
    Document::AllocatorType& allocator = report.GetAllocator();

    // Select the function
    size_t select_func = random.get<uint16_t>() % func_count;
    auto& the_func = funcs[select_func];
    auto args = GenerateArgs(the_func.parameters, random);

    // Log the execution
    reportArr.AddMember(Value("FunctionName"),
                        Value(the_func.function_name.c_str(), allocator).Move(),
                        allocator);
    Value argArray(kArrayType);
    for(auto& arg : args) {
      argArray.PushBack(Value(StringBinRepresentation(arg).c_str(), allocator).Move(), allocator);
    }
    reportArr.AddMember(Value("Args"), argArray.Move(), allocator);

    // Invoke
    auto [res, elapsed] = InvokeFunction(the_func.function_name, args);
    
    
    reportArr.AddMember(Value("Elapsed"),
                        Value(std::to_string(elapsed).c_str(), allocator).Move(), 
                        allocator);

    if(res.has_value()) {
      reportArr.AddMember(Value("Success"), Value(true), allocator);
      if(res->type != WasmType::Void)
        reportArr.AddMember(Value("Result"),
                            Value(StringBinRepresentation(*res).c_str(), allocator).Move(), 
                            allocator);
    } else {
      reportArr.AddMember(Value("Success"), Value(false), allocator);
    }

    if(memory.has_value()) {
      auto comparison = this->CompareInternalMemory(*memory);
      if(!comparison.empty()) {
        Value memDiff(kObjectType);
        for(auto& diff : comparison) {
          Value thisDiff(kArrayType);
          thisDiff.PushBack(Value(diff.old_byte), allocator);
          thisDiff.PushBack(Value(diff.new_byte), allocator);
          memDiff.AddMember(Value(std::to_string(diff.index).c_str(), allocator).Move(), thisDiff.Move(), allocator);
        }
        reportArr.AddMember(Value("MemoryDiff"), memDiff.Move(), allocator);
      }
    }
    
    // Do comparison globals
    Value globalDiff(kObjectType);
    int global_diff_ctr = 0;
    for(auto& global : global_state) {
      auto new_state = GetGlobal(global.first);
      if(new_state != global.second) {
        Value thisDiff(kArrayType);
        thisDiff.PushBack(Value(StringBinRepresentation(global.second).c_str(), allocator).Move(), allocator);
        thisDiff.PushBack(Value(StringBinRepresentation(new_state).c_str(), allocator).Move(), allocator);
        globalDiff.AddMember(Value(&global.first[6], allocator).Move(), thisDiff.Move(), allocator);
        global_diff_ctr++;
          
        global.second = new_state;
      }
    }
    if(global_diff_ctr != 0) {
      reportArr.AddMember(Value("GlobalDiff"), globalDiff.Move(), allocator);
    }

    OStreamWrapper osw(*output);
    Writer<OStreamWrapper> writer(osw);
    report.Accept(writer);
  }
  *output << "]";

  return true;
}

bool dfw::FuzzerRunnerBase::InvokeFunction(dfw::FuzzerRunnerCLArgs const& args) {
  if(!args.function.set) {
    std::cerr << "Set the function name through -function args\n";
    return false;
  }

  std::optional<std::vector<uint8_t>> memory;
  std::cout << "Load memory" << std::endl;
  if(args.memory.set) {
    memory.emplace(LoadMemory(args.memory.value));
  }
  std::cout << "Initialize execution" << std::endl;
  RETURN_IF_FALSE(InitializeExecution());

  // Loop function call, cover all possible functions
  std::cout << "Get functions" << std::endl;
  DataRange random { *memory };
  decltype(auto) funcs = Functions();
  auto selectedFunc = std::find_if(funcs.begin(), funcs.end(), [&args] (dfw::FunctionInfo const& f) {
                        return f.function_name == args.function.value;
                      });

  if(selectedFunc == funcs.end())
    return false;

  std::cout << "Start Invoking\n" << std::endl;
  /* Remnants?
  std::sort(funcs.begin(), funcs.end(), 
            [] (dfw::FunctionInfo const& a, dfw::FunctionInfo const& b) {
              return a.function_name > b.function_name;
            });
  */
  std::cout << "[INVOKE] " 
            << selectedFunc->function_name << " " 
            << selectedFunc->parameters.size() << std::endl;
  std::cout.flush();
  auto [res, elapsed] = InvokeFunction(selectedFunc->function_name, 
                                       GenerateArgs(selectedFunc->parameters, random));
  
  if(res.has_value()) {
    std::cout << "[INVOKE DONE] success: "; 
    PrintJSValue(*res);
  } else {
    std::cout << "[INVOKE DONE] failed\n";
  }

  std::cout << std::endl;
  return true;
}

std::vector<dfw::JSValue> dfw::FuzzerRunnerBase::GenerateArgs(std::vector<WasmType> const& param_types, DataRange& random) {
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
  std::cout.flush();
  return args;
}

int dfw::FuzzerRunnerBase::Run(int argc, char const* argv[]) {
  dfw::FuzzerRunnerCLArgs args { argc, argv };

  ERROR_IF_FALSE(InitializeModule(args), "Failed initializing and compiling WASM");
  
  if(std::strcmp(args.mode, "interactive") == 0) {
    Looper();
  } else if(std::strcmp(args.mode, "single") == 0) {
    ERROR_IF_FALSE(SingleRun(args), "Failed executing test case.");
  } else if(std::strcmp(args.mode, "invoke") == 0) {
    ERROR_IF_FALSE(InvokeFunction(args), "Failed invoking function.");
  }
  return 0;
}

dfw::FuzzerRunnerBase::~FuzzerRunnerBase() {
  
}