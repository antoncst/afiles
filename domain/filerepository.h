#ifndef FILEREPOSITORY_H
#define FILEREPOSITORY_H

#include "filenode.h"
#include <vector>
#include <string>
#include <QString>
#include <cstdint>
namespace domain::repositories {

class IFileRepository {
public:
    virtual ~IFileRepository() = default;

    virtual std::vector<domain::entities::FileNode> scanDirectory(
        const QString& path,
        int parentIndex = -1
        ) = 0;

    virtual bool createDirectory(const std::string& path) = 0;
    virtual bool remove(const std::string& path) = 0;
    virtual bool rename(const std::string& oldPath, const std::string& newPath) = 0;
    virtual bool copy(const std::string& source, const std::string& destination) = 0;
    virtual bool exists(const std::string& path) = 0;

    virtual std::string getFileName(const std::string& path) = 0;
    virtual uint64_t getFileSize(const std::string& path) = 0;
    virtual uint64_t getModifiedTime(const std::string& path) = 0;
    virtual uint64_t getCreatedTime(const std::string& path) = 0;
    virtual uint32_t getPermissions(const std::string& path) = 0;
    virtual domain::entities::FileType getFileType(const std::string& path) = 0;
};

} // namespace domain::repositories


#endif // FILEREPOSITORY_H
