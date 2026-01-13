#include <modules/builder/builder_plugin.h>
#include <modules/builder/builder.h>

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

static int run(const std::string& cmd) {
    std::cout << cmd << std::endl;
    return std::system(cmd.c_str());
}

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::lib(ctx, api, {}, {}, false);
    builder_t::lib(ctx, api, {}, {}, true);
}

// ---------- ESP32 change detection ----------

static bool is_ignored(const fs::path& p) {
    const auto name = p.filename().string();
    return name == "build" ||
           name == "managed_components" ||
           name == ".idf_py_cache";
}

static std::time_t to_time_t(fs::file_time_type ft) {
    using namespace std::chrono;
    auto sctp = time_point_cast<seconds>(
        ft - fs::file_time_type::clock::now()
        + system_clock::now()
    );
    return system_clock::to_time_t(sctp);
}

static std::time_t max_mtime_recursive(const fs::path& dir) {
    std::time_t t = 0;
    for (auto& e : fs::recursive_directory_iterator(dir)) {
        if (is_ignored(e.path())) continue;
        if (!fs::is_regular_file(e.status())) continue;
        t = std::max(t, to_time_t(fs::last_write_time(e)));
    }
    return t;
}

static bool needs_rebuild(const fs::path& src, const fs::path& stamp) {
    if (!fs::exists(stamp)) return true;

    std::ifstream in(stamp);
    std::time_t old_ts = 0;
    in >> old_ts;

    return max_mtime_recursive(src) > old_ts;
}

static void write_stamp(const fs::path& src, const fs::path& stamp) {
    std::ofstream out(stamp, std::ios::trunc);
    out << max_mtime_recursive(src);
}

// ---------- ESP32 build ----------

static void build_esp32(builder_ctx_t* ctx, const builder_api_t* api, const char* /*static_libs*/) {
    const fs::path modules_dir  = api->modules_dir(ctx);
    const fs::path source_dir   = fs::absolute(api->source_dir(ctx));
    const fs::path artifact_dir = api->artifact_dir(ctx);

    const fs::path esp32_dir  = source_dir / "esp32";
    const fs::path artifacts = artifact_dir / "esp32";
    fs::create_directories(artifacts);

    // stable toolchain location (workspace-level)
    const fs::path workspace_root = fs::absolute(modules_dir).parent_path();
    const fs::path toolchains_dir = workspace_root / ".toolchains";
    const fs::path esp_idf_dir    = toolchains_dir / "esp-idf";
    fs::create_directories(toolchains_dir);

    // install ESP-IDF if missing
    if (!fs::exists(esp_idf_dir / "export.sh")) {
        if (run("cd " + toolchains_dir.string() +
                " && git clone --depth=1 https://github.com/espressif/esp-idf.git") != 0)
            throw std::runtime_error("clone esp-idf failed");

        if (run("cd " + esp_idf_dir.string() + " && ./install.sh esp32") != 0)
            throw std::runtime_error("install esp-idf tools failed");
    }

    // ensure websocket component
    {
        const fs::path yml = esp32_dir / "idf_component.yml";
        bool need = true;
        if (fs::exists(yml)) {
            std::ifstream in(yml);
            std::string s((std::istreambuf_iterator<char>(in)), {});
            if (s.find("esp_websocket_client") != std::string::npos)
                need = false;
        }
        if (need) {
            std::string cmd =
                "bash -c 'source " + (esp_idf_dir / "export.sh").string() +
                " >/dev/null && cd " + esp32_dir.string() +
                " && idf.py add-dependency \"espressif/esp_websocket_client\" || true'";
            run(cmd);
        }
    }

    const fs::path stamp = artifacts / ".stamp";
    const bool rebuild = needs_rebuild(esp32_dir, stamp);

    if (rebuild) {
        std::cout << "[esp32] sources changed, rebuilding\n";
        std::string cmd =
            "bash -c 'source " + (esp_idf_dir / "export.sh").string() +
            " >/dev/null && cd " + esp32_dir.string() +
            " && idf.py build'";
        if (run(cmd) != 0)
            throw std::runtime_error("idf.py build failed");

        write_stamp(esp32_dir, stamp);
    } else {
        std::cout << "[esp32] unchanged, reusing cached build\n";
    }

    // ---------- ALWAYS re-emit artifacts ----------
    const fs::path build = esp32_dir / "build";
    if (!fs::exists(build))
        throw std::runtime_error("ESP32 build directory missing");

    fs::copy_file(build / "bootloader" / "bootloader.bin",
                  artifacts / "bootloader.bin",
                  fs::copy_options::overwrite_existing);

    fs::copy_file(build / "partition_table" / "partition-table.bin",
                  artifacts / "partition-table.bin",
                  fs::copy_options::overwrite_existing);

    bool found_app = false;
    for (auto& e : fs::directory_iterator(build)) {
        if (e.path().extension() == ".bin" &&
            e.path().filename() != "bootloader.bin" &&
            e.path().filename() != "partition-table.bin") {
            fs::copy_file(e.path(), artifacts / "app.bin",
                          fs::copy_options::overwrite_existing);
            found_app = true;
            break;
        }
    }

    if (!found_app)
        throw std::runtime_error("ESP32 app.bin not found");

    // flash helper
    fs::path flash = artifacts / "flash.sh";
    {
        FILE* f = std::fopen(flash.c_str(), "w");
        std::fprintf(f,
            "#!/bin/sh\n"
            "source %s/export.sh >/dev/null\n"
            "cd %s\n"
            "idf.py -p ${PORT:-/dev/ttyUSB0} -b 115200 flash\n",
            esp_idf_dir.c_str(),
            esp32_dir.c_str());
        std::fclose(f);
    }
    fs::permissions(flash, fs::perms::owner_exec, fs::perm_options::add);
}

// ---------- server artifact ----------

static void build_server(builder_ctx_t* ctx, const builder_api_t* api, const char* /*static_libs*/) {
    const fs::path source_dir   = fs::absolute(api->source_dir(ctx));
    const fs::path artifact_dir = api->artifact_dir(ctx);

    const fs::path server_src  = source_dir / "server";
    const fs::path artifacts   = artifact_dir / "server";
    const fs::path venv        = artifacts / "venv";

    fs::create_directories(artifacts);

    // copy server source
    fs::copy_file(
        server_src / "server.py",
        artifacts / "server.py",
        fs::copy_options::overwrite_existing
    );

    // create venv if missing
    if (!fs::exists(venv / "bin/python")) {
        if (run("python3 -m venv " + venv.string()) != 0)
            throw std::runtime_error("failed to create venv");

        if (run((venv / "bin/pip").string() + " install websockets") != 0)
            throw std::runtime_error("failed to install websockets");
    }

    // run helper
    {
        FILE* f = std::fopen((artifacts / "run_server.sh").c_str(), "w");
        std::fprintf(f,
            "#!/bin/sh\n"
            "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n"
            "exec \"$DIR/venv/bin/python\" \"$DIR/server.py\"\n");
        std::fclose(f);
    }

    fs::permissions(
        artifacts / "run_server.sh",
        fs::perms::owner_exec,
        fs::perm_options::add
    );
}
// ---------- module entry ----------

BUILDER_EXTERN void builder__build_module(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const char* static_libs
) {
    build_esp32(ctx, api, static_libs);
    build_server(ctx, api, static_libs);
}
