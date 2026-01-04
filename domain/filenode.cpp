
// ENTITY

#include "filenode.h"
#include <sstream>
#include <QDir>

namespace domain::entities {

QString FileNode::getFullPath(const std::vector<FileNode>& fileSystem) const {
    if (isRoot()) {
        return name;
    }
    QString full_name = name ;
    int index = parentIndex ;

    //const tfile * cur_dir = m_files[ m_parent_dir ] ;
    while ( index != -1 )
    {
        QString nname = fileSystem[ index ].name ;
        if ( fileSystem[ index ].parentIndex != -1 )
            nname += QDir::separator() ;
        full_name = nname + full_name ;
        index = fileSystem[ index ].parentIndex ;
    } ;

    return full_name ;
}


QString FileNode::getParentPath(const std::vector<FileNode>& fileSystem) const {
    if (isRoot() || parentIndex == -1) {
        return "";
    }

    if (parentIndex >= 0 && parentIndex < static_cast<int>(fileSystem.size())) {
        return fileSystem[parentIndex].getFullPath(fileSystem);
    }

    return "";
}

} // namespace domain::entities
