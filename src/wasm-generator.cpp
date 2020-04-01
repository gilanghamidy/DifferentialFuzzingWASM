// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libplatform/libplatform.h"
#include "v8.h"
#include "v8-ext.h"

#include <iostream>

#include "runner-common.h"

int main(int argc, char* argv[]) {
  if(argc < 4) {
    std::cerr << "Need argument: [file output] [random size] [random seed]." << std::endl;
    return 1;
  }
  uint64_t randomSize = 0;
  uint64_t randomSeed = 0;
  {
    std::string randomSizeStr { argv[2] },
                randomSeedStr { argv[3] };

    try {
      randomSize = std::stoll(randomSizeStr);
      randomSeed = std::stoll(randomSeedStr);
    } catch(std::invalid_argument const& e) {
      std::cerr << "Random size or random seed argument is invalid." << std::endl;
      return 2;
    }
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
      std::cout << "Generating WASM...\n";
      std::vector<uint8_t> randomizedData = dfw::GenerateRandomData(randomSeed, randomSize);
      std::vector<uint8_t> generatedWasm;

      v8::ext::GenerateRandomWasm(isolate, randomizedData, generatedWasm);
      std::cout << "Writing WASM...\n";
      dfw::WriteOutput(argv[1], generatedWasm);
      //std::for_each(randomizedData.begin(), randomizedData.end(), [&] (uint8_t p) { std::cout << (uint32_t)p << " "; });
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;
  return 0;
}
