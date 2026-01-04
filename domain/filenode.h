
// ENTITY

#ifndef FILENODE_H
#define FILENODE_H

#include <QString>
#include <vector>
#include <cstdint>
//#include <chrono>

namespace domain::entities {

enum class FileType : uint8_t {
    Directory
    , File
    //, Symlink
};

#pragma pack(push, 1)

struct FileNode {
    //int index = -1;
    int parentIndex = -1;
    //std::vector<int> childrenIndices;

    // Красиво использовать std::string, но жертвуем универсальностью в пользу
    // производительности, и, главное, использованию Qt::CaseInsensitive
    QString name;           // Только имя файла, не полный путь
    FileType type;
    //uint64_t size = 0;
    //uint32_t permissions = 0;

    // Время в секундах с эпохи для экономии памяти
    //uint64_t modifiedTime = 0;
    //uint64_t createdTime = 0;

    bool isRoot() const { return parentIndex == -1; }
    bool isDirectory() const { return type == FileType::Directory; }
    bool isFile() const { return type == FileType::File; }

    // Методы для вычисления полного пути
    QString getFullPath(const std::vector<FileNode>& fileSystem) const;
    QString getParentPath(const std::vector<FileNode>& fileSystem) const;
};

#pragma pack(pop)

} // namespace domain::entities

#endif // FILENODE_H
