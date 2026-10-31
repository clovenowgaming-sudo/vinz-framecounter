#include "MacroIO.hpp"

#include <gdr/gdr.hpp>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <span>

namespace fs = std::filesystem;

namespace {
    // GDR2 replay type used by this mod.
    struct FCReplay : gdr::Replay<FCReplay, gdr::Input<>> {
        FCReplay() : Replay("FrameCounter", 1) {}
    };

    fs::path downloadDir() {
        return fs::path("/storage/emulated/0/Download");
    }

    std::string safeName(std::string const& in) {
        std::string out;
        for (char c : in) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) out += c;
            else if (c == ' ' || c == '_' || c == '-') out += '_';
        }
        if (out.empty()) out = "macro";
        if (out.size() > 24) out.resize(24);
        return out;
    }
}

namespace macroio {

fs::path macroDir() {
    auto dir = Mod::get()->getSaveDir() / "macros";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

std::vector<fs::path> listMacros() {
    std::vector<fs::path> out;
    std::vector<fs::path> dirs = {macroDir(), downloadDir()};
    for (auto const& dir : dirs) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;
        for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            std::error_code ec2;
            if (it->is_regular_file(ec2) && it->path().extension() == ".gdr2") {
                out.push_back(it->path());
            }
        }
    }
    std::sort(out.begin(), out.end(), [](fs::path const& a, fs::path const& b) {
        return a.filename().string() < b.filename().string();
    });
    return out;
}

bool exportMacro(std::vector<ClickInput> const& inputs, std::string const& levelName,
                 int levelId, std::string& info) {
    if (inputs.empty()) {
        info = "Nothing to export";
        return false;
    }

    FCReplay replay;
    replay.author = "FrameCounter";
    replay.description = "Recorded with Frame Window Counter";
    replay.gameVersion = GEODE_COMP_GD_VERSION;
    replay.framerate = 240.0;
    replay.levelInfo.id = static_cast<uint32_t>(levelId);
    replay.levelInfo.name = levelName;

    int lastTick = 0;
    for (auto const& in : inputs) {
        replay.inputs.push_back(gdr::Input<>(
            static_cast<uint64_t>(std::max(0, in.tick)),
            static_cast<uint8_t>(in.button),
            !in.p1,
            in.down
        ));
        lastTick = std::max(lastTick, in.tick);
    }
    replay.duration = static_cast<float>(lastTick) / 240.f;

    auto res = replay.exportData();
    if (res.isErr()) {
        info = "GDR export failed";
        return false;
    }
    auto data = res.unwrap();

    std::string fileName = fmt::format("{}_{}.gdr2", safeName(levelName), static_cast<long long>(std::time(nullptr)));
    auto path = macroDir() / fileName;
    {
        std::ofstream f(path, std::ios::binary);
        if (!f) {
            info = "Cannot write " + path.string();
            return false;
        }
        f.write(reinterpret_cast<char const*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    info = path.string();

    // Best effort copy to Download so it is easy to find.
    std::error_code ec;
    if (fs::is_directory(downloadDir(), ec)) {
        fs::copy_file(path, downloadDir() / fileName, fs::copy_options::overwrite_existing, ec);
    }
    return true;
}

bool importMacro(fs::path const& path, std::vector<ClickInput>& out, std::string& err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        err = "Cannot open file";
        return false;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.size() < 4 || bytes[0] != 'G' || bytes[1] != 'D' || bytes[2] != 'R') {
        err = "Not a GDR2 file";
        return false;
    }

    auto res = FCReplay::importData(std::span<uint8_t>(bytes.data(), bytes.size()));
    if (res.isErr()) {
        err = "GDR import failed";
        return false;
    }
    auto replay = res.unwrap();

    out.clear();
    for (auto const& in : replay.inputs) {
        out.push_back(ClickInput{
            static_cast<int>(in.frame),
            static_cast<int>(in.button),
            in.down,
            !in.player2
        });
    }
    std::stable_sort(out.begin(), out.end(), [](ClickInput const& a, ClickInput const& b) {
        return a.tick < b.tick;
    });
    if (out.empty()) {
        err = "Macro has no inputs";
        return false;
    }
    return true;
}

}
