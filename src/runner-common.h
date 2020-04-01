#ifndef RUNNER_COMMON_H
#define RUNNER_COMMON_H

#include <vector>
#include <cstdint>
#include <ostream>
#include <string>
#include <optional>
#include <functional>

namespace dfw {
    std::vector<uint8_t> OpenInput(char const* fileName);
    void DumpDisassemble(std::ostream& output, std::vector<uint8_t> const& instructions);
    void WriteOutput(char const* fileName, std::vector<uint8_t> const& buf);
    std::vector<uint8_t> GenerateRandomData(uint64_t seed, uint32_t len);

    typedef std::optional<std::vector<uint8_t>> FuncNameToBinFunctor(std::string const&);
    void DumperLooper(std::function<FuncNameToBinFunctor> transform);
}



#endif