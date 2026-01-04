#include "qtfilerepository.h"
#include <QDirIterator>

namespace infrastructure::persistence {

std::vector<domain::entities::FileNode> QtFileRepository::scanDirectory(
    const QString& path, int parentIndex) // не будем использовать, сканируем директорию в FileProc
{

    std::vector<domain::entities::FileNode> nodes;

    QDir dir( path );
    if (!dir.exists()) return nodes;

    auto entries = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
        QDir::Name | QDir::DirsFirst
        );

    for (const auto& entry : entries) {
        domain::entities::FileNode node;
        node.parentIndex = parentIndex;
        node.name = entry.fileName() ;
        node.type = qFileInfoToFileType(entry);
        //node.size = static_cast<uint64_t>(entry.size());
        //node.modifiedTime = entry.lastModified().toSecsSinceEpoch();
        //node.createdTime = entry.birthTime().isValid() ?
        //                       entry.birthTime().toSecsSinceEpoch() :
        //                       entry.lastModified().toSecsSinceEpoch();
        //node.permissions = static_cast<uint32_t>(entry.permissions());

        nodes.push_back(node);
    }

    return nodes;
}

bool QtFileRepository::createDirectory(const std::string& path) {
    QDir dir;
    return dir.mkpath(QString::fromStdString(path));
}

bool QtFileRepository::remove(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    if (fileInfo.isDir()) {
        QDir dir(QString::fromStdString(path));
        return dir.removeRecursively();
    } else {
        QFile file(QString::fromStdString(path));
        return file.remove();
    }
}

bool QtFileRepository::rename(const std::string& oldPath, const std::string& newPath) {
    QFile file(QString::fromStdString(oldPath));
    return file.rename(QString::fromStdString(newPath));
}

bool QtFileRepository::copy(const std::string& source, const std::string& destination) {
    return QFile::copy(
        QString::fromStdString(source),
        QString::fromStdString(destination)
        );
}

bool QtFileRepository::exists(const std::string& path) {
    return QFileInfo::exists(QString::fromStdString(path));
}

std::string QtFileRepository::getFileName(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return fileInfo.fileName().toStdString();
}

uint64_t QtFileRepository::getFileSize(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return static_cast<uint64_t>(fileInfo.size());
}

uint64_t QtFileRepository::getModifiedTime(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return fileInfo.lastModified().toSecsSinceEpoch();
}

uint64_t QtFileRepository::getCreatedTime(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return fileInfo.birthTime().isValid() ?
               fileInfo.birthTime().toSecsSinceEpoch() :
               fileInfo.lastModified().toSecsSinceEpoch();
}

uint32_t QtFileRepository::getPermissions(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return static_cast<uint32_t>(fileInfo.permissions());
}

domain::entities::FileType QtFileRepository::getFileType(const std::string& path) {
    QFileInfo fileInfo(QString::fromStdString(path));
    return qFileInfoToFileType(fileInfo);
}

domain::entities::FileType QtFileRepository::qFileInfoToFileType(const QFileInfo& fileInfo) const {
    if (fileInfo.isDir()) return domain::entities::FileType::Directory;
    //if (fileInfo.isSymLink()) return domain::entities::FileType::Symlink;
    return domain::entities::FileType::File;
}

} // namespace infrastructure::persistence
