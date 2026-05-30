#include "text.hpp"

namespace focusforge::utils {

std::string PluralRu(int n, const std::string& one, const std::string& few,
                     const std::string& many) {
    int mod10 = n % 10, mod100 = n % 100;
    if (mod100 >= 11 && mod100 <= 19)
        return std::to_string(n) + " " + many;
    if (mod10 == 1)
        return std::to_string(n) + " " + one;
    if (mod10 >= 2 && mod10 <= 4)
        return std::to_string(n) + " " + few;
    return std::to_string(n) + " " + many;
}

}  // namespace focusforge::utils
