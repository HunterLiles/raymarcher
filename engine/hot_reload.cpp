#include "hot_reload.hpp"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <spawn.h>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

extern char **environ;

HotReload::~HotReload() {
    std::error_code error;

    if (!work.empty())
        std::filesystem::remove_all(work, error);
}

void HotReload::configure(const std::filesystem::path &shader,
                          const std::filesystem::path &definitions,
                          const std::filesystem::path &tool) {
    source = std::filesystem::absolute(shader);
    sdf = std::filesystem::absolute(definitions);
    compiler = tool;

    auto pattern = (std::filesystem::temp_directory_path() / "engine-shaders-XXXXXX").string();

    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');

    char *directory = mkdtemp(name.data());

    if (!directory)
        throw std::system_error(errno, std::generic_category());

    work = directory;
    observed = attempted = scan();
}

HotReload::Files HotReload::scan() const {
    Files files;
    auto options = std::filesystem::directory_options::skip_permission_denied;

    for (const auto &root : {
             source.parent_path(),
             sdf.parent_path(),
             source.parent_path().parent_path() / "engine",
         }) {
        std::error_code error;

        if (!std::filesystem::exists(root, error))
            continue;

        for (auto it = std::filesystem::recursive_directory_iterator(root, options, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            const auto &file = *it;
            auto ext = file.path().extension();

            if (!file.is_regular_file(error) || (ext != ".glsl" && ext != ".inc" && ext != ".h"))
                continue;

            auto time = file.last_write_time(error);
            if (error)
                continue;

            auto size = file.file_size(error);
            if (!error)
                files[file.path()] = {time, size};
        }
    }

    return files;
}

bool HotReload::changed() {
    if (!enabled() || Clock::now() < next_check)
        return false;

    next_check = Clock::now() + std::chrono::milliseconds(250);
    auto current = scan();

    if (current != observed) {
        observed = std::move(current);
        return false;
    }

    return observed != attempted;
}

void HotReload::run(std::vector<std::string> args) const {
    std::vector<char *> argv;

    for (auto &arg : args)
        argv.push_back(arg.data());

    argv.push_back(nullptr);
    auto log = work / "compiler.log";

    posix_spawn_file_actions_t actions;
    int result = posix_spawn_file_actions_init(&actions);

    if (result)
        throw std::system_error(result, std::generic_category());

    result = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log.c_str(),
                                              O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (!result)
        result = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

    pid_t pid{};

    if (!result)
        result = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);

    posix_spawn_file_actions_destroy(&actions);

    if (result)
        throw std::system_error(result, std::generic_category());

    int status{};

    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            throw std::system_error(errno, std::generic_category());

    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
        std::ifstream input(log);

        throw std::runtime_error("glslc failed:\n" +
                                 std::string(std::istreambuf_iterator<char>(input), {}));
    }
}

ShaderBuild HotReload::build() {
    observed = attempted = scan();

    auto wrapper = work / "metadata.comp.glsl";
    auto rows = work / "metadata.txt";

    {
        std::ofstream file(wrapper);

        file << "#version 460\n#define SDF_CACHE_LIST_ONLY\n"
                "#define ivec3(x,y,z) x y z\n"
                "#define CACHE_SDF(id,name,lo,hi,size,fps) id size fps\n"
                "#include "
             << std::quoted(sdf.filename().string()) << '\n';

        if (!file)
            throw std::runtime_error("Cannot write shader metadata wrapper");
    }

    run({
        compiler.string(),
        "-E",
        "-fshader-stage=compute",
        "-I" + sdf.parent_path().string(),
        wrapper.string(),
        "-o",
        rows.string(),
    });

    ShaderBuild output{work / "render.spv", work / "cache.spv", {}};
    std::ifstream input(rows);

    for (std::string line; std::getline(input, line);) {
        auto first = line.find_first_not_of(" \t\r");

        if (first == std::string::npos || line[first] == '#')
            continue;

        std::istringstream row(line);
        uint64_t id{}, x{}, y{}, z{};
        float fps{};
        std::string extra;

        if (!(row >> id >> x >> y >> z >> fps) || (row >> extra) || id > UINT32_MAX || x < 2 ||
            y < 2 || z < 2 || x > UINT32_MAX || y > UINT32_MAX || z > UINT32_MAX ||
            x * y > UINT32_MAX / z || !std::isfinite(fps) || fps < 0)
            throw std::runtime_error("Invalid CACHE_SDF declaration: " + line);

        output.caches.push_back({
            uint32_t(id),
            uint32_t(x * y * z),
            fps,
        });
    }

    run({
        compiler.string(),
        "-fshader-stage=compute",
        "--target-env=vulkan1.3",
        "-Werror",
        "-c",
        source.string(),
        "-o",
        output.render.string(),
    });

    run({
        compiler.string(),
        "-fshader-stage=compute",
        "--target-env=vulkan1.3",
        "-Werror",
        "-DSDF_CACHE_UPDATE=1",
        "-c",
        source.string(),
        "-o",
        output.cache.string(),
    });

    return output;
}
