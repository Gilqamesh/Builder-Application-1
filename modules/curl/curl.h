#ifndef MODULES_CURL_H
# define MODULES_CURL_H

# include <filesystem>
# include <string>

class curl_t {
public:
    static std::filesystem::path download(const std::string& url, const std::filesystem::path& install_path);
};

#endif // MODULES_CURL_H
