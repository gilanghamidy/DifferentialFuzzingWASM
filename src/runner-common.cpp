#include "runner-common.h"

#include <random>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ext/stdio_filebuf.h>

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
  int i = 0;
  while(!is.eof()) {
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
    
    auto func = transform(input); //(*res)[input];
    
    if(!func) {
      std::cout << "Unknown function!\n";
      continue;
    }

    dfw::DumpDisassemble(std::cout, func.value());
  }
}