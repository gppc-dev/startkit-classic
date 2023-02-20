#ifndef INX_VALIDATEPATH_HPP_INCLUDED
#define INX_VALIDATEPATH_HPP_INCLUDED

#include "BresenhamRay.hpp"

namespace inx {

std::unique_ptr<BresenhamRay>& ValidatePath_data()
{
    static std::unique_ptr<BresenhamRay> rayShooter;
    return rayShooter;
}

// returns -1 if valid path, otherwise id of segment where invalidness was detetcted
// map is loaded only ONCE, if has changed, will error
// can call ValidatePath_data().reset() to allow new map
template <typename T>
int ValidatePath(const std::vector<bool>& map, int width, int height, const T& path)
{
    auto& rayShooter = ValidatePath_data();
    if (rayShooter == nullptr) {
        rayShooter = std::make_unique<BresenhamRay>();
        rayShooter->setGrid<true>(static_cast<size_t>(width), static_cast<size_t>(height), map);
    }
    return rayShooter->validPath(path);
}

} // namespace inx

#endif // INX_VALIDATEPATH_HPP_INCLUDED
