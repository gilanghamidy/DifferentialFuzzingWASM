#include "jsapi.h"
#include "jsapi-ext.h"
#include "js/Initialization.h"
#include "js/CompilationAndEvaluation.h"
#include "js/SourceText.h"
#include "js/Conversions.h"
#include "js/Modules.h"
#include <iostream>
#include <fstream>
#include <memory>

#include "runner-common.h"

namespace {
  class RunnerSpiderMonkey {
  private:
    JSContext* context;
    std::unique_ptr<js::ext::CompiledInstructions> compiled_wasm;
    std::vector<dfw::FunctionInfo> functions;
    std::vector<dfw::GlobalInfo> globals;

    //dfw::JSValue MarshallValue(v8::Local<v8::Value> const& ref);
    //std::vector<v8::Local<v8::Value>> MarshallArgs(std::vector<dfw::JSValue> const& args);
  public:
    RunnerSpiderMonkey(JSContext* context) : context(context) { }

    bool InitializeModule(dfw::FuzzerRunnerCLArgs args) {
      auto inputInstruction = dfw::OpenInput(args.input);
      this->compiled_wasm = js::ext::CompileWasmBytes(context, inputInstruction.data(), 
                                                      inputInstruction.size());
      
      if(this->compiled_wasm == nullptr)
        return false;

      // Fill reflection information
      for(auto& func : this->compiled_wasm->functions()) {
        dfw::FunctionInfo info;
        info.function_name = func.name;
        info.return_type = (dfw::WasmType)func.returnType;
        info.instruction_address = func.instruction_address;
        for(auto param : func.parameters)
          info.parameters.push_back((dfw::WasmType)param);
        
        this->functions.emplace_back(std::move(info));
      }

      this->compiled_wasm->NewGlobalImport(this->context);

      for(auto& global : this->compiled_wasm->Globals()) {
        dfw::GlobalInfo info;
        info.global_name = global.first;
        info.type = (dfw::WasmType)global.second;

        this->globals.emplace_back(std::move(info));
      }

      return true;
    }

    std::optional<std::vector<uint8_t>> DumpFunction(std::string const& name) {
      auto func = (*this->compiled_wasm)[name];
      if(func)
        return std::make_optional(func->instructions);
      else
        return std::nullopt;
    }


    bool MarshallMemoryImport(uint8_t* source, size_t len) {
      this->compiled_wasm->NewMemoryImport(this->context);
      auto wasmMem = this->compiled_wasm->GetWasmMemory();

      // Check if buffer is instantiated
      if(wasmMem.buffer == nullptr)
        return false;

      // Marshall the buffer as needed
      // If source is smaller than WASM memory, only fill the memory up to the available source
      // and the remainder will be undefined
      // If source is bigger than WASM memory, only fill to the available WASM memory and the
      // remainder is discarded
      auto buffer = wasmMem.buffer;
      for(int i = 0; i < len && i < wasmMem.length; ++i) {
        buffer[i] = source[i];
      }
        

      return true;
    }

    uintptr_t GetWasmMemoryAddress() {
      auto wasmMem = this->compiled_wasm->GetWasmMemory();
      return (uintptr_t)wasmMem.buffer;
    }

    bool InitializeExecution() {
      return this->compiled_wasm->InstantiateWasm(this->context);
    }

    

    void MarshallArgs(std::vector<JS::Value>& ret, std::vector<dfw::JSValue> const& args) {
      for(auto& arg : args) {
        switch(arg.type) {
          case dfw::WasmType::I32: {
            //std::cout << "i32: " << arg.i32 << " ";
            //dfw::PrintHexRepresentation(arg.i32);
            ret.emplace_back(JS::Int32Value(arg.i32));
            break;
          }
          case dfw::WasmType::I64: {
            //std::cout << "i64: " << arg.i64 << " ";
            //dfw::PrintHexRepresentation(arg.i64);
            ret.emplace_back(js::ext::CreateBigIntValue(this->context, arg.i64));
            break;
          }
          case dfw::WasmType::F32: {
            double d = arg.f32; // Implicit conversion first
            //std::cout << "f32: " << arg.f32 << " ";
            //dfw::PrintHexRepresentation(d);
            ret.emplace_back(JS::DoubleValue(d));
            break;
          }
          case dfw::WasmType::F64: {
            //std::cout << "f64: " << arg.f64 << " ";
            //dfw::PrintHexRepresentation(arg.f64);
            ret.emplace_back(JS::DoubleValue(arg.f64));
            break;
          }
          default:
            break;
        }
        //std::cout << std::endl;
      }
    }

    dfw::JSValue MarshallValue(JS::Value const& ref) {
  
      dfw::JSValue ret;
      
      if(ref.isInt32()) {
        ret.type = dfw::WasmType::I32;
        ret.i32 = ref.toInt32();
      } else if(ref.isBigInt()) {
        ret.type = dfw::WasmType::I64;
        ret.i64 = js::ext::GetBigIntValue(ref);
      } else if(ref.isDouble()) {
        ret.type = dfw::WasmType::F64;
        ret.f64 = ref.toDouble();
      } else if(ref.isUndefined()){
        ret.type = dfw::WasmType::Void;
      } else {
        std::cerr << "Unexpected marshalling!" << std::endl;
        std::abort();
      }

      return ret;
    }

    std::tuple<std::optional<dfw::JSValue>, uint64_t> InvokeFunction(std::string const& name, std::vector<dfw::JSValue> const& args) {
      
      
      std::vector<JS::Value> callStack;
      callStack.emplace_back(); // Return value
      callStack.emplace_back(); // MAGIC (empty)
      MarshallArgs(callStack, args);
      std::cout.flush();
      auto [invokeRes, elapsed] = (*compiled_wasm)[name]->Invoke(context, callStack);
      
      if(!invokeRes)
        return {std::nullopt, elapsed};
      else
        return {std::optional<dfw::JSValue> { MarshallValue(callStack[0]) }, elapsed };
    }
    std::vector<dfw::FunctionInfo> const& Functions() const { return functions; }

    ~RunnerSpiderMonkey() {

    }

    std::vector<dfw::GlobalInfo> Globals() {
      return this->globals;
    }

    void SetGlobal(std::string const& arg, dfw::JSValue val) {
      js::ext::CompiledInstructions::WasmGlobalArg global_arg;
      using dfw::WasmType;
      switch(val.type) {
        case WasmType::I32: {
          global_arg.i32 = val.i32;
          break;
        }
        case WasmType::I64: {
          global_arg.i64 = val.i64;
          break;
        }
        case WasmType::F32: {
          global_arg.f32 = val.f32;
          break;
        }
        case WasmType::F64: {
          global_arg.f64 = val.f64;
          break;
        }
        default: break;
      }
      
      this->compiled_wasm->SetGlobalImport(this->context, arg, global_arg);
    }

    dfw::JSValue GetGlobal(std::string const& arg) {
      auto res = this->compiled_wasm->GetGlobalImport(this->context, arg);
      using E = js::ext::WasmType;
      dfw::JSValue ret;
      ret.type = (dfw::WasmType)res.first;
      switch(res.first) {
        case E::I32: ret.i32 = res.second.i32; break;
        case E::I64: ret.i64 = res.second.i64; break;
        case E::F32: ret.f32 = res.second.f32; break;
        case E::F64: ret.f64 = res.second.f64; break;
        case E::Void: break;
      }
      return ret;
    }

    std::vector<dfw::MemoryDiff> CompareInternalMemory(std::vector<uint8_t>& buffer) {
      auto ref = this->compiled_wasm->GetWasmMemory();
      std::vector<dfw::MemoryDiff> ret;
      for(uint32_t i = 0; i < std::min(ref.length, buffer.size()); ++i) {
        if(buffer[i] != ref.buffer[i]) {
          ret.emplace_back(i, buffer[i], ref.buffer[i]);
          buffer[i] = ref.buffer[i]; // Update the buffer
        }
      }
      return ret;
    }
  };
}


/* The class of the global object. */
static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &JS::DefaultGlobalClassOps
};

int main(int argc, const char *argv[])
{
  int ret = 0;
  JS_Init();

  JSContext *cx = JS_NewContext(8L * 1024 * 1024);
  if (!cx)
      return 1;
  if (!JS::InitSelfHostedCode(cx))
      return 1;

  { // Scope for our various stack objects (JSAutoRequest, RootedObject), so they all go
    // out of scope before we JS_DestroyContext.

    JS::RealmOptions options;
    JS::RootedObject global(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::FireOnNewGlobalHook, options));
    if (!global)
        return 1;

    JS::RootedValue rval(cx);

    { // Scope for JSAutoCompartment
      JSAutoRealm ac(cx, global);
      JS::InitRealmStandardClasses(cx);

      ret = dfw::FuzzerRunner<RunnerSpiderMonkey>{cx}.Run(argc, argv);

      
      /*
      dfw::DumperLooper([&] (auto arg) { 
        
        return func 
                ? std::make_optional(func->instructions) 
                : std::nullopt;
      });

      
      std::vector<JS::Value> args;
      
      args.emplace_back(JS::Int32Value(25));
      args.emplace_back();
      args.emplace_back();
      args.emplace_back(js::ext::CreateBigIntValue(cx, 300ll));
      //std::cout << "Param 1: " << args[2].toInt32() << std::endl;
      bool invokeRes = (*res)["subi64"].Invoke(cx, args);

      std::cout << "Invoke result: " << invokeRes << std::endl;
      JS::Value& val = args[0];*/
      //std::cout << "Func result: " << js::ext::GetBigIntValue(val) /*.toInt32()*/ << std::endl;
      

      /*
      for(auto& func : res->functions()) {
        std::cout << "Function Name: " << func.name << "\nInstructions: \n";
        DumpDisassemble(std::cout, func.instructions);
      }*/
    }
  }

  JS_DestroyContext(cx);
  JS_ShutDown();
  return ret;
}