#include <pybind11/pybind11.h>
#include "ValidatePath.hpp"

namespace py = pybind11;

struct xyLoc {
    double x;
    double y;
};

struct Checker{
    std::vector<bool> map;
    int width;
    int height;
    Checker(py::list& theMap, int width, int height):width(width),height(height)
    {
        map.resize(py::len(theMap));
        for(int i = 0 ; i<py::len(theMap);i++){
            map[i] = theMap[i].cast<bool>();
        }

    }
    int validatePath(py::list thePath){
        std::vector<xyLoc> path;
        path.resize(py::len(thePath));
        for(int i=0;i<py::len(thePath);i++){
            xyLoc loc;
            loc.x = thePath[i].attr("x").cast<double>();
            loc.y = thePath[i].attr("y").cast<double>();
            path[i] = loc;
        }

        return inx::ValidatePath(map, width, height, path);
    };
};




PYBIND11_MODULE(Anyangle_Path_Checker, m) {
    py::class_<Checker>(m, "Anyangle_Path_Checker")
        .def(py::init<py::list&, int, int>())
        .def("validatePath", &Checker::validatePath);
}


