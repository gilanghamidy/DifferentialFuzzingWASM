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

