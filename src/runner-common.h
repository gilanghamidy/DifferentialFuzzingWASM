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
#include <cmath>
#include <limits>
#include <set>

#define COMMON_FILE_DESCRIPTOR 3

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
  bool set{true};
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
  dfw::CommandLineArg<char const*> function { "-function", false };
  dfw::CommandLineArg<int> count { "-invoke-count", false, 50 };
  char const* exec_path;
  FuzzerRunnerCLArgs(int argc, char const* argv[]) : exec_path(argv[0]) {
    dfw::CommandLineConsumer { argc, argv, 
                               std::ref(input),
                               std::ref(mode),
                               std::ref(memory),
                               std::ref(function) };
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

struct GlobalInfo {
  std::string global_name;
  WasmType type;
};

struct JSValue {
  WasmType type;
  union {
    uint32_t i32;
    uint64_t i64;
    float    f32;
    double   f64;
  };

  bool operator==(JSValue const& that) const;

  bool operator!=(JSValue const& that) const {
    return !(*this == that);
  }
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

template<typename T, size_t typeSize = sizeof(T)>
struct StorageSelector;

template<typename T>
struct StorageSelector<T, sizeof(int32_t)> {
  using Type = int32_t;
};

template<typename T>
struct StorageSelector<T, sizeof(int64_t)> {
  using Type = int64_t;
};

template<typename T>
using StorageSelectorT = typename StorageSelector<T>::Type;

template<>
inline bool DataRange::get<bool>() {
  return get<uint8_t>() % 2 == 0;
}

template<>
inline double DataRange::get<double>() {
  // Try getting value first
  uint64_t temp = get<uint64_t>();
  double val = *reinterpret_cast<double*>(&temp);
  if(std::isnan(val))
    return std::numeric_limits<double>::quiet_NaN();
  else
    return val;
}

template<>
inline float DataRange::get<float>() {
  // Try getting value first
  uint32_t temp = get<uint32_t>();
  float val = *reinterpret_cast<float*>(&temp);
  if(std::isnan(val))
    return std::numeric_limits<float>::quiet_NaN();
  else
    return val;
}

struct MemoryDiff {
  uint32_t index;
  uint8_t old_byte;
  uint8_t new_byte;

  MemoryDiff(uint32_t index, uint8_t old_byte, uint8_t new_byte) :
    index(index), old_byte(old_byte), new_byte(new_byte) { }
};



void PrintJSValue(JSValue const& v);

class FuzzerRunnerBase {
public:
  void Looper();
  std::vector<uint8_t> LoadMemory(char const* memfile);
  std::vector<dfw::JSValue> GenerateArgs(std::vector<WasmType> const& param_types, DataRange& random);
  bool SingleRun(dfw::FuzzerRunnerCLArgs const& args);
  bool InvokeFunction(dfw::FuzzerRunnerCLArgs const& args);
  int Run(int argc, char const* argv[]);

  virtual std::vector<FunctionInfo> const& Functions() = 0;
  virtual std::optional<std::vector<uint8_t>> DumpFunction(std::string const&) = 0;
  virtual std::tuple<std::optional<dfw::JSValue>, uint64_t> InvokeFunction(std::string const&, std::vector<JSValue> const&) = 0;
  virtual bool InitializeExecution() = 0;
  virtual bool InitializeModule(dfw::FuzzerRunnerCLArgs const&) = 0;
  virtual bool MarshallMemoryImport(uint8_t*, size_t) = 0;
  virtual std::vector<MemoryDiff> CompareInternalMemory(std::vector<uint8_t>& buffer) = 0;
  virtual std::vector<GlobalInfo> Globals() = 0;
  virtual void SetGlobal(std::string const& arg, JSValue value) = 0;
  virtual JSValue GetGlobal(std::string const& arg) = 0;
  virtual ~FuzzerRunnerBase();
};

template<typename T>
class FuzzerRunner : public FuzzerRunnerBase {
  T runner;
public:
  template<typename... Args>
  FuzzerRunner(Args&&... a) : runner(std::forward<Args>(a)...) { }

  virtual std::vector<FunctionInfo> const& Functions() {
    return runner.Functions();
  }

  virtual std::optional<std::vector<uint8_t>> DumpFunction(std::string const& f) {
    return runner.DumpFunction(f);
  }

  virtual std::tuple<std::optional<dfw::JSValue>, uint64_t> InvokeFunction(std::string const& f, std::vector<JSValue> const& args) {
    return runner.InvokeFunction(f, args);
  }

  virtual bool InitializeExecution() {
    return runner.InitializeExecution();
  }

  virtual bool InitializeModule(dfw::FuzzerRunnerCLArgs const& a) {
    return runner.InitializeModule(a);
  }

  virtual bool MarshallMemoryImport(uint8_t* m, size_t s) {
    return runner.MarshallMemoryImport(m, s);
  }

  virtual std::vector<GlobalInfo> Globals() {
    return runner.Globals();
  }

  virtual void SetGlobal(std::string const& arg, JSValue value) {
    runner.SetGlobal(arg, value);
  }

  virtual JSValue GetGlobal(std::string const& arg) {
    return runner.GetGlobal(arg);
  }

  virtual std::vector<MemoryDiff> CompareInternalMemory(std::vector<uint8_t>& buffer) {
    return runner.CompareInternalMemory(buffer);
  }
};



template<typename T>
void PrintHexRepresentation(T v) {
  StorageSelectorT<T>* ptr = reinterpret_cast<StorageSelectorT<T>*>(&v);
  std::cout << "(0x" << std::hex << *ptr << ")";
  std::cout << std::dec;
}

template<typename T>
std::string StringHexRepresentation(T v) {
  std::stringstream ss;
  StorageSelectorT<T> temp;
  std::memset(&temp, 0, sizeof(temp));
  std::memcpy(&temp, &v, sizeof(v));
  ss << "0x" << std::hex << temp;
  return ss.str();
}

inline std::string StringHexRepresentation(JSValue v) {
  switch (v.type)
  {
  case WasmType::I32: return StringHexRepresentation(v.i32);
  case WasmType::I64: return StringHexRepresentation(v.i64);
  case WasmType::F32: return StringHexRepresentation(v.f32);
  case WasmType::F64: return StringHexRepresentation(v.f64);
  default:
    return "";
  }
}

template<typename T>
int64_t BinRepresentation(T v) {
  StorageSelectorT<T> temp;
  std::memset(&temp, 0, sizeof(temp));
  std::memcpy(&temp, &v, sizeof(v));
  return temp;
}

inline int64_t BinRepresentation(JSValue v) {
  switch (v.type)
  {
  case WasmType::I32: return BinRepresentation(v.i32);
  case WasmType::I64: return BinRepresentation(v.i64);
  case WasmType::F32: return BinRepresentation(v.f32);
  case WasmType::F64: return BinRepresentation(v.f64);
  default:
    return 0;
  }
}

inline std::string StringBinRepresentation(JSValue v) {
  return std::to_string(BinRepresentation(v));
}

template<typename T, typename... TArgs>
void strjoin_loop(std::string& s, T arg, TArgs... args) {
  s.append(arg);
  if constexpr(sizeof...(args) != 0)
    strjoin_loop(s, args...);
}

template<typename... TArgs>
std::string strjoin(TArgs... args) {
  std::string ret;
  strjoin_loop(ret, args...);
  return ret;
}

inline bool JSValue::operator==(JSValue const& that) const {
  if(this->type != that.type) {
    std::cerr << "Diff type\n";
    return false;
  }
    
  
  switch(this->type) {
    case WasmType::I32: return this->i32 == that.i32;
    case WasmType::I64: return this->i64 == that.i64;
    case WasmType::F32: return (std::isnan(this->f32) && std::isnan(that.f32)) || this->f32 == that.f32;
    case WasmType::F64: return (std::isnan(this->f64) && std::isnan(that.f64)) || this->f64 == that.f64;
    default:            return true;
  }

  return true;
}

} // namespace dfw

#endif