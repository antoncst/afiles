#ifndef DIRSPATHS_H
#define DIRSPATHS_H

#include <string>
#include <vector>

namespace domain::entities {

struct dirs_paths {
    std::vector<std::string> roots ;
    std::vector<std::string> exclude ;
};

} // domain::entities

#endif // DIRSPATHS_H
