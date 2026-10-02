#include "shaders.hpp"
#include <fstream>
#include <vector>
VkShaderModule shader_load(VkDevice device, const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Cannot open shader: " + path.string());
    auto bytes = file.tellg();
    if (bytes <= 0 || bytes % 4)
        throw std::runtime_error("Invalid SPIR-V size: " + path.string());
    std::vector<uint32_t> code(size_t(bytes) / 4);
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(code.data()), bytes))
        throw std::runtime_error("Cannot read shader: " + path.string());
    if (code[0] != 0x07230203)
        throw std::runtime_error("Invalid SPIR-V header");
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = size_t(bytes);
    ci.pCode = code.data();
    VkShaderModule module{};
    VK_CHECK(vkCreateShaderModule(device, &ci, nullptr, &module));
    return module;
}
