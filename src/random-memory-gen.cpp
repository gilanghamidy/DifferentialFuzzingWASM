#include <random>
#include <iterator>
#include <vector>
#include <fstream>

void GenerateMemory(std::string output_file,
                    size_t mem_size,
                    uint64_t seed) {
  // Generate random memory
  std::vector<uint32_t> mem_page_buffer;
  mem_page_buffer.resize(64 * 1024 / sizeof(uint32_t));
  
  std::mt19937 re_mem;
  re_mem.seed(seed);

  // Generate n bytes of memory output
  std::ofstream mem_output(output_file, std::ios::out);

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

void GenerateZeroMemory(std::string output_file, size_t mem_size) {
  // Generate random memory
  
  std::vector<uint32_t> mem_page_buffer;
  mem_page_buffer.resize(64 * 1024 / sizeof(uint32_t));
  for(int i = 0; i < mem_page_buffer.size(); ++i) {
    mem_page_buffer[i] = 0;
  }

  // Generate n bytes of memory output
  std::ofstream mem_output(output_file, std::ios::out);

  for(int i = 0; i < mem_size; ++i) {

    // Write one block
    mem_output.write((char const*)mem_page_buffer.data(),
                      mem_page_buffer.size() * sizeof(decltype(mem_page_buffer)::value_type));
  }
}

void GenerateOneMemory(std::string output_file, size_t mem_size) {
  // Generate random memory  
  std::vector<uint32_t> mem_page_buffer;
  mem_page_buffer.resize(64 * 1024 / sizeof(uint32_t));
  
  // Generate one block
  for(int i = 0; i < mem_page_buffer.size(); ++i) {
    mem_page_buffer[i] = 0x01010101;
  }

  // Generate n bytes of memory output
  std::ofstream mem_output(output_file, std::ios::out);

  for(int i = 0; i < mem_size; ++i) {
    // Write one block
    mem_output.write((char const*)mem_page_buffer.data(),
                      mem_page_buffer.size() * sizeof(decltype(mem_page_buffer)::value_type));
  }
}


int main(int argc, char const* argv[]) {
  GenerateZeroMemory("zero.mem", 10);
  GenerateOneMemory("one.mem", 10);
  GenerateMemory("rand1.mem", 10, 1234567890);
  GenerateMemory("rand2.mem", 10, 2468012345);
  GenerateMemory("rand3.mem", 10, 5432101234);
}
