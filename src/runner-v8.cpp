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

int main(int argc, char* argv[]) {
  if(argc < 2) {
    std::cerr << "Need argument file input." << std::endl;
    return 1;
  }

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
      auto bsource = dfw::OpenInput(argv[1]);
      printf("Trying Compiling Binary WASM\n");
      v8::Maybe<v8::ext::CompiledWasm> res = v8::ext::CompileBinaryWasm(isolate, bsource.data(), bsource.size());

      if(!res.IsNothing()) {
        /*
        printf("Trying Calling WASM\n");
        v8::ext::CompiledWasm compiled = res.ToChecked();
        auto& func = compiled.FunctionByName("main");
        std::vector<v8::Local<v8::Value>> args;
        args.emplace_back(v8::Int32::New(isolate, 2147483647));
        args.emplace_back(v8::Int32::New(isolate, 2147483648));
        v8::MaybeLocal<v8::Value> ret = func.Invoke(isolate, args);

        if(ret.IsEmpty()) {
          printf("Failed calling");
        } else {
          auto hasil = ret.ToLocalChecked()->Int32Value(context).ToChecked();
          printf("Success calling %d\n", hasil);
        }*/
        auto compiled = res.ToChecked();
        dfw::DumperLooper([&] (std::string const& arg) ->
          std::optional<std::vector<uint8_t>> {
          try {
            auto& func = compiled.FunctionByName(arg);
            return std::make_optional(func.Instructions());
          } catch(std::exception const& e) {
            return std::nullopt;
          }
        });
      }
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}
