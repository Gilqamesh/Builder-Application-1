#include <modules/curl/curl.h>
#include <curl/curl.h>

#include <format>
#include <fstream>

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ofstream* ofs = static_cast<std::ofstream*>(userdata);
    ofs->write(ptr, size * nmemb);
}

std::filesystem::path curl_t::download_file(const std::string& url, const std::filesystem::path& install_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("failed to initialize curl");
    }

    if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::format("failed to set URL '{}'", url));
    }

    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::format("failed to set write callback for URL '{}'", url));
    }

    std::ofstream ofs;
    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs) != CURLE_OK) {
    }

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::format("failed to download file from URL '{}': {}", url, curl_easy_strerror(result)));
    }
    curl_easy_cleanup(curl);
}
