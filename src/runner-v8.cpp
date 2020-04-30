// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libplatform/libplatform.h"
#include "v8.h"
#include "v8-ext.h"

#include <iostream>
#include <fstream>
#include <vector>

#include "runner-common.h"

class RunnerV8 {
private:
  v8::Isolate* isolate;
  v8::Local<v8::Context>& context;
  v8::ext::CompiledWasm compiled_wasm;
  std::vector<dfw::FunctionInfo> functions;

  dfw::JSValue MarshallValue(v8::Local<v8::Value> const& ref);
  std::vector<v8::Local<v8::Value>> MarshallArgs(std::vector<dfw::JSValue> const& args);
public:
  RunnerV8(v8::Isolate* isolate, v8::Local<v8::Context>& context) :
    isolate(isolate), context(context) { }

  bool InitializeModule(dfw::FuzzerRunnerCLArgs args);
  std::optional<std::vector<uint8_t>> DumpFunction(std::string const& name);
  bool MarshallMemoryImport(uint8_t* source, size_t len);
  bool InitializeExecution();
  std::optional<dfw::JSValue> InvokeFunction(std::string const& name, std::vector<dfw::JSValue> const& args);
  std::vector<dfw::FunctionInfo> const& Functions() const { return functions; }

  ~RunnerV8();
};


bool RunnerV8::InitializeModule(dfw::FuzzerRunnerCLArgs args) {
  // Start Compiling
  auto bsource = dfw::OpenInput(args.input);
  v8::Maybe<v8::ext::CompiledWasm> res = 
      v8::ext::CompileBinaryWasm(isolate, bsource.data(), bsource.size());
  
  // Store the compiled WASM locally
  if(!res.IsNothing()) {
    this->compiled_wasm = res.ToChecked();

    // Fill reflection information
    for(auto& func : this->compiled_wasm.Functions()) {
      dfw::FunctionInfo info;
      info.function_name = func.Name();
      info.return_type = (dfw::WasmType)func.ReturnType();
      
      for(auto param : func.Parameters())
        info.parameters.push_back((dfw::WasmType)param);
      
      this->functions.emplace_back(std::move(info));
    }

    return true;
  } else {
    return false;
  }
}

std::optional<std::vector<uint8_t>> RunnerV8::DumpFunction(std::string const& name) {
  try {
    auto& func = this->compiled_wasm.FunctionByName(name);
    return std::make_optional(func.Instructions());
  } catch(std::exception const& e) {
    return std::nullopt;
  }
}

bool RunnerV8::MarshallMemoryImport(uint8_t* source, size_t len) {
  // Init memory import
  this->compiled_wasm.NewMemoryImport(isolate);
  auto wasmMem = this->compiled_wasm.GetWasmMemory();

  // Check if buffer is instantiated
  if(wasmMem.buffer == nullptr)
    return false;

  // Marshall the buffer as needed
  // If source is smaller than WASM memory, only fill the memory up to the available source
  // and the remainder will be undefined
  // If source is bigger than WASM memory, only fill to the available WASM memory and the
  // remainder is discarded
  auto buffer = wasmMem.buffer.get();
  for(int i = 0; i < len && i < wasmMem.length; ++i)
    buffer[i] = source[i];

  return true;
}

bool RunnerV8::InitializeExecution() {
  // Instantiate WASM module
  return this->compiled_wasm.InstantiateWasm(isolate);
}

dfw::JSValue RunnerV8::MarshallValue(v8::Local<v8::Value> const& ref) {
  
  dfw::JSValue ret;
  //if(ref->IsUndefined()) {
    //ret.type = dfw::WasmType::Void;
  //} else 
  if(ref->IsUint32()) {
    ret.type = dfw::WasmType::I32;
    ret.i32 = ref->Uint32Value(this->context).ToChecked();
  } else if(ref->IsInt32()) {
    ret.type = dfw::WasmType::I32;
    ret.i32 = ref->Int32Value(this->context).ToChecked();
  } else if(ref->IsBigInt()) {
    ret.type = dfw::WasmType::I64;
    ret.i64 = ref->ToBigInt(this->context).ToLocalChecked()->Int64Value();
  } else if(ref->IsNumber()) {
    ret.type = dfw::WasmType::F64;
    ret.f64 = ref->ToNumber(this->context).ToLocalChecked()->Value();
  } else {
    ret.type = dfw::WasmType::Void;
    //std::cerr << "Unexpected marshalling!" << std::endl;
    //std::abort();
  }

  return ret;
}

std::vector<v8::Local<v8::Value>> RunnerV8::MarshallArgs(std::vector<dfw::JSValue> const& args) {
  std::vector<v8::Local<v8::Value>> ret;
  for(auto& arg : args) {
    switch(arg.type) {
      case dfw::WasmType::I32: {
        ret.emplace_back(v8::Int32::NewFromUnsigned(this->isolate, arg.i32));
        std::cout << "i32 ";
        break;
      }
      case dfw::WasmType::I64: {
        ret.emplace_back(v8::BigInt::NewFromUnsigned(this->isolate, arg.i64));
        std::cout << "i64 ";
        break;
      }
      case dfw::WasmType::F32: {
        ret.emplace_back(v8::Number::New(this->isolate, arg.f32));
        std::cout << "f32 ";
        break;
      }
      case dfw::WasmType::F64: {
        ret.emplace_back(v8::Number::New(this->isolate, arg.f64));
        std::cout << "f64 ";
        break;
      }
      default:
        break;
    }
  }
  return ret;
}

std::optional<dfw::JSValue> RunnerV8::InvokeFunction(std::string const& name, std::vector<dfw::JSValue> const& args) {
  auto& funcMain = this->compiled_wasm.FunctionByName(name);
  
  // Marshall the argument
  std::vector<v8::Local<v8::Value>> args_marshalled = MarshallArgs(args);
  v8::MaybeLocal<v8::Value> ret = funcMain.Invoke(isolate, args_marshalled);
  
  if(ret.IsEmpty()) {
    return std::nullopt;
  } else {
    auto hasil = MarshallValue(ret.ToLocalChecked());
    return std::make_optional<dfw::JSValue>(hasil);
  }
}

RunnerV8::~RunnerV8() {

}


int main(int argc, char const* argv[]) {
  int ret = 0;

  // Initialize V8.
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    {
      ret = dfw::FuzzerRunner<RunnerV8>{isolate, context}.Run(argc, argv);

      
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}
