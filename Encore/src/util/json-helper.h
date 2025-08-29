//
// Created by maria on 17/12/2024.
//

#ifndef JSON_HELPER_H
#define JSON_HELPER_H
#include <fstream>
#include <nlohmann/json.hpp>

namespace Encore {
    inline void WriteJsonFile(const std::filesystem::path &FileToWrite, const nlohmann::json &JSONobject) {
        std::ofstream o(FileToWrite, std::ios::out | std::ios::trunc);
        o << JSONobject.dump(2, ' ', false, nlohmann::detail::error_handler_t::strict);
        o.close();
    }
}
#endif //JSON_HELPER_H
