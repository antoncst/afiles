
// SERVICE

#include "filesproc.h"
#include <algorithm>
#include <iostream>

namespace domain::services {

FilesProc::FilesProc(std::unique_ptr<repositories::IFileRepository> repository)
    : repository(std::move(repository)) {}

std::vector<entities::FileNode> FilesProc::scanFileSystem(
    const ::application::dto::DirsConfig dirs_config)
{
    std::vector<entities::FileNode> fileSystem; // создаём fileSystem
    int totalScanned = 0;
    QElapsedTimer progressTimer;
    progressTimer.start();

    // Создаем контекст
    ScanContext context{
        .fileSystem = fileSystem,
        .dirs_config = dirs_config,
        .totalScanned = totalScanned,
        .progressTimer = progressTimer,
        //.parent = this
    };

    for (const auto& rootPath : context.dirs_config.roots) {
        if (!repository->exists(rootPath.toStdString())) {
            continue;
        }

        // Создаем корневой узел
        entities::FileNode rootNode;
        //rootNode.index = nextIndex++;
        rootNode.parentIndex = -1;
        rootNode.name = rootPath ;
        rootNode.type = repository->getFileType(rootPath.toStdString()) ;
        //rootNode.size = repository->getFileSize(rootPath);
        //rootNode.modifiedTime = repository->getModifiedTime(rootPath);
        //rootNode.createdTime = repository->getCreatedTime(rootPath);
        //rootNode.permissions = repository->getPermissions(rootPath);

        context.fileSystem.push_back(rootNode);
        ++context.totalScanned ;

        // Рекурсивно сканируем содержимое
        scanDirectoryRecursive(context, rootPath, fileSystem.size() - 1 );
    }

    return fileSystem; // возвращаем вектор
}


void FilesProc::scanDirectoryRecursive(
    ScanContext& context, // Принимает по ссылке
    const QString & path,
    int parentIndex)
{
    if ( stop_scan )
        return ;

    if (context.progressTimer.elapsed() >= 1000 ) { // Каждые 1000 мс
        emit scanProgress( context.totalScanned, path ) ;
        context.progressTimer.restart();
    }

    //QString path_norm = ! path.endsWith( QDir::separator() ) && ! path.endsWith( '/' )
    //                        ? path + QDir::separator() : path ;

    QDir dir( path );
    if ( !dir.exists() ) return ;

    auto entries = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks
        //, QDir::DirsFirst // QDir::Name |
        );

    for (const auto& entry : entries)
    {
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


        //Решено:  Корневой path - содержит слэш в конце
        // а при рекурсии path не содержит слэш в конце, поэтому надо его добавлять в path
        QString subdirpath ;
        if ( node.isDirectory() )
        {
            if ( path.endsWith( QDir::separator() ) )
                subdirpath = path + node.name ;
            else
                subdirpath = path + QDir::separator() + node.name ;

            if ( context.shouldExclude( subdirpath ) )
                continue ;
        }
            /*        //skipping dirs
        bool bSkipDirFound = false;
        for ( QString & skip_dir :  )
            if ( (path_norm + subdir).contains( skip_dir ) )
            {
                bSkipDirFound = true ;
                umap_undiscovered_skip_dirs_[ skip_dir ] = true ;
                break ;
            }
        if ( bSkipDirFound )
            continue ;
        // end of skipping dirs
*/

        context.fileSystem.push_back(node);
        ++context.totalScanned ;
        // Рекурсивно сканируем поддиректории
        if (node.isDirectory())
        {
            scanDirectoryRecursive(context
                                   , subdirpath
                                   , context.fileSystem.size() - 1 );
        }
    }

    //auto children = repository->scanDirectory(path_norm, parentIndex);

    //for (auto& child : children) {
    //}
}

bool FilesProc::ScanContext::shouldExclude(const QString& path) const {
    for (const auto& exclude : dirs_config.exclude) {
        if (path.contains(exclude, Qt::CaseInsensitive)) {
            //umap_undiscovered_skip_dirs_[ skip_dir ] = true ;
            return true;
        }
    }
    return false;
}


QString normalize_str( QString const & str )
{
    QString res = str.toLower() ;

    // non-strict matching :
    for ( auto & ch : res )
    {
        if ( ch == QChar(L'ё') )
            ch = QChar(L'е') ;
        else if ( ch == QChar(L'y') || ch == QChar(L'j') )
            ch = QChar(L'i') ;
        else if ( ch == QChar(L'c') )
            ch = QChar(L'k') ;
    }

    return res ;
}


std::vector<int> FilesProc::searchFiles(
    const std::vector<entities::FileNode>& fileSystem,
    const QString & searchTerm) {

    std::vector<int> resultIndices;

    size_t index = 0 ;
    for (const auto& node : fileSystem)
    {
        if ( node.name.contains(searchTerm , Qt::CaseInsensitive) )
            resultIndices.push_back(index);
        ++index ;
    }

    return resultIndices;
}

domain::entities::FileType FilesProc::qFileInfoToFileType(const QFileInfo& fileInfo) const {
    if (fileInfo.isDir()) return domain::entities::FileType::Directory;
    //if (fileInfo.isSymLink()) return domain::entities::FileType::Symlink;
    return domain::entities::FileType::File;
}

/*int FileWorker::addNode(const entities::FileNode& node) {
    entities::FileNode newNode = node;
    newNode.index = nextIndex++;
    fileSystem.push_back(newNode);
    return newNode.index;
}
*/
} // namespace domain::services
