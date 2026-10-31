#pragma once
#include "CounterState.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace macroio {
    // <save dir>/macros
    std::filesystem::path macroDir();

    // All .gdr2 files in the macros folder and in /storage/emulated/0/Download
    std::vector<std::filesystem::path> listMacros();

    // Writes a .gdr2 file. info = path on success, error text on failure.
    bool exportMacro(std::vector<ClickInput> const& inputs, std::string const& levelName,
                     int levelId, std::string& info);

    // Reads a .gdr2 file. err = error text on failure.
    bool importMacro(std::filesystem::path const& path, std::vector<ClickInput>& out, std::string& err);
}
