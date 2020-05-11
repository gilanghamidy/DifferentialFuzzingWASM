
#include "runner-common.h"

#include <sys/stat.h> 
#include <sys/types.h> 

#include <fcntl.h>
#include <unistd.h> 

#include <ext/stdio_filebuf.h>
#include <random>
#include <iterator>

#include "libplatform/libplatform.h"
#include "v8-ext.h"

struct CommandLineArgument {
  dfw::CommandLineArg<uint64_t> randomSize { "-block-size", true };
  dfw::CommandLineArg<uint64_t> randomSeed { "-seed", true };
  dfw::CommandLineArg<bool> repro { "-repro" };
  dfw::CommandLineArg<uint64_t> skipCount { "-skip-count" };
  dfw::CommandLineArg<uint64_t> skipMemoryCount { "-skip-memory-count" };
  dfw::CommandLineArg<char const*> outfile { "-output", true };
  dfw::CommandLineArg<char const*> memory { "-memory", true };

  CommandLineArgument(int argc, char const* argv[]) {
    dfw::CommandLineConsumer { argc, argv, 
                          std::ref(randomSize),
                          std::ref(randomSeed),
                          std::ref(skipCount),
                          std::ref(outfile),
                          std::ref(memory),
                          std::ref(skipMemoryCount),
                          std::ref(repro) };
  }
};

size_t GenerateRandomWASM(CommandLineArgument& args, 
                        std::vector<uint8_t>& randomizedData,
                        std::mt19937& re,
                        v8::Isolate* isolate) {
  // Generate the random WASM
  auto dataBeginAsInt32 = (uint32_t*) randomizedData.data();
  auto dataEndAsInt32 = dataBeginAsInt32 + randomizedData.size() / sizeof(uint32_t);

  std::for_each(dataBeginAsInt32, 
                dataEndAsInt32, 
                [&] (uint32_t& val) { val = re(); });

  {
    std::ofstream output_bin("random.bin", std::ios::out);
    output_bin.write((char const*)randomizedData.data(), randomizedData.size());
  }
  
  std::cout << "Generating WASM...\n";
  std::vector<uint8_t> generatedWasm;

  std::ofstream output(args.outfile, std::ios::out);
  auto [success, mem_size_ret] = v8::ext::GenerateRandomWasm(isolate, randomizedData, generatedWasm);

  std::cout << "Memory size: " << mem_size_ret << std::endl;
  
  if(!success) {
    std::cerr << "Error generating WASM!\n";
    std::abort();
  }

  std::cout << "Writing WASM...\n";

  output.write((char const*)generatedWasm.data(), generatedWasm.size());
  output.flush();

  return mem_size_ret;
}
std::vector<uint32_t> mem_page_buffer;


void GenerateMemory(CommandLineArgument& args, 
                    std::vector<uint8_t>& randomizedData, 
                    size_t mem_seed_ptr,
                    size_t mem_size) {
  // Generate random memory

  // Reseed the mem randomizer
  auto dataBeginAsInt32 = (uint32_t*) randomizedData.data();
  auto maxIndex = randomizedData.size() / sizeof(uint32_t);
  auto seed = dataBeginAsInt32[mem_seed_ptr % maxIndex];

  std::mt19937 re_mem;
  re_mem.seed(seed);

  // Generate n bytes of memory output
  std::ofstream mem_output(args.memory, std::ios::out);

  for(int i = 0; i < mem_size; ++i) {
    // Generate one block
    for(int i = 0; i < mem_page_buffer.size(); ++i) {
      mem_page_buffer[i] = re_mem();
    }
    // Write one block
    mem_output.write((char const*)mem_page_buffer.data(),
                      mem_page_buffer.size() * sizeof(decltype(mem_page_buffer)::value_type));
  }
}

int main(int argc, char const* argv[]) {
  CommandLineArgument args { argc, argv };

  // Prepare buffer
  mem_page_buffer.resize(64 * 1024 / sizeof(uint32_t)); // Single page WASM memory

  std::mt19937 re(args.randomSeed);

  size_t mem_seed_ptr;
  size_t mem_size;
  

  if(args.skipCount != 0) {
    int skippedByteCount = args.skipCount * args.randomSize / sizeof(uint32_t);
    for(int i = 0; i < skippedByteCount; ++i)
      re();
  }

  std::vector<uint8_t> randomizedData;
  randomizedData.resize(args.randomSize);
  std::string input;

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
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    if(!args.repro) {
      while(true) {
        std::getline(std::cin, input);
        if(input == "q") 
          break;
        else if(input == "w") {
          mem_size = GenerateRandomWASM(args, randomizedData, re, isolate);
          // Reset mem seed
          mem_seed_ptr = 0;
        } else if(input == "m") {
          GenerateMemory(args, randomizedData, mem_seed_ptr, mem_size);
          ++mem_seed_ptr;
        }
      }
    } else {
      mem_size = GenerateRandomWASM(args, randomizedData, re, isolate);
      GenerateMemory(args, randomizedData, args.skipMemoryCount, mem_size);
    }
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete create_params.array_buffer_allocator;

  return 0;
}