
// qt filesystem adapter

#ifndef QTFILEREPOSITORY_H
#define QTFILEREPOSITORY_H

#include "../../domain/filerepository.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QString>

namespace infrastructure::persistence {

class QtFileRepository : public domain::repositories::IFileRepository {
public:
    std::vector<domain::entities::FileNode> scanDirectory(const QString &path,
        int parentIndex = -1
        ) override;

    bool createDirectory(const std::string& path) override;
    bool remove(const std::string& path) override;
    bool rename(const std::string& oldPath, const std::string& newPath) override;
    bool copy(const std::string& source, const std::string& destination) override;
    bool exists(const std::string &path) override;

    std::string getFileName(const std::string& path) override;
    uint64_t getFileSize(const std::string& path) override;
    uint64_t getModifiedTime(const std::string& path) override;
    uint64_t getCreatedTime(const std::string& path) override;
    uint32_t getPermissions(const std::string& path) override;
    domain::entities::FileType getFileType(const std::string& path) override;

private:
    domain::entities::FileType qFileInfoToFileType(const QFileInfo& fileInfo) const;
};

} // namespace infrastructure::persistence


#endif // QTFILEREPOSITORY_H
