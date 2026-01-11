
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
    ScanContext & context,
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

    auto file_mask = QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks ;
    file_mask = context.dirs_config.show_hidden ? file_mask | QDir::Hidden : file_mask ;

    QDir dir( path );
    if ( !dir.exists() ) return ;

    for (const QString & filename : dir.entryList( file_mask ) )
    {

        domain::entities::FileNode node;
        node.parentIndex = parentIndex;
        node.name = filename ;
        node.type = domain::entities::FileType::File ; //qFileInfoToFileType(entry);
        //node.size = static_cast<uint64_t>(entry.size());
        //node.modifiedTime = entry.lastModified().toSecsSinceEpoch();
        //node.createdTime = entry.birthTime().isValid() ?
        //                       entry.birthTime().toSecsSinceEpoch() :
        //                       entry.lastModified().toSecsSinceEpoch();
        //node.permissions = static_cast<uint32_t>(entry.permissions());
        context.fileSystem.push_back(node);
        ++context.totalScanned ;
        if ( context.totalScanned % 1000 == 0 )
            if ( stop_scan )
                return ;
    }

    auto dir_mask = QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks ;
    dir_mask =  context.dirs_config.show_hidden ? dir_mask | QDir::Hidden : dir_mask ;

    for ( QString const & subdir : dir.entryList( dir_mask ) )
    {
        //Решено:  Корневой path - содержит слэш в конце
        // а при рекурсии path не содержит слэш в конце, поэтому надо его добавлять в path
        QString subdirpath ;
        if ( path.endsWith( QDir::separator() ) )
            subdirpath = path + subdir ;
        else
            subdirpath = path + QDir::separator() + subdir ;

        if ( context.shouldExclude( subdirpath ) ) {
            //umap_undiscovered_skip_dirs_[ skip_dir ] = true ; //todo
            continue ;
        }

        domain::entities::FileNode node;
        node.parentIndex = parentIndex;
        node.name = subdir ;
        node.type = domain::entities::FileType::Directory ; //qFileInfoToFileType(entry);
        context.fileSystem.push_back(node);
        ++context.totalScanned ;
        // Рекурсивно сканируем поддиректории
        scanDirectoryRecursive(context
                               , subdirpath
                               , context.fileSystem.size() - 1 );
    }

    //Как должно было быть в чистой архитектуре (отказались ради производительности):
    //auto children = repository->scanDirectory(path_norm, parentIndex);
    //for (auto& child : children) {
    //}
}

// C:/Program Files; c:\windows; AppData; c:\qt;
bool FilesProc::ScanContext::shouldExclude(const QString& path) const {
    QString normalizedPath = path.toLower();

    for (const auto& exclude : dirs_config.exclude) {
        //проверка на абсолютный путь
#ifdef Q_OS_WIN
        if ( exclude.length() >= 2 && ( exclude[1] == ':' ) ) // i.e. c:\path
            if ( normalizedPath.startsWith(exclude) )
                return true ;
            else
                continue ;
        if ( exclude.length() >= 1 && ( exclude[0] == ':' ) ) // i.e. :\path
            if ( QStringView(normalizedPath).sliced(1).startsWith(exclude) )
                return true ;
            else
                continue ;
#elif
        if ( exclude[0] = '/' )
            if ( normalizedPath.startsWith(exclude) )
                return true;
            else
                continue ;
#endif
        //относительный путь
        // Проверяем только после каждого слэша
        int pos = 0;
            while ((pos = normalizedPath.indexOf(QDir::separator(), pos + 1)) != -1) {
            if (QStringView(normalizedPath).sliced(pos + 1).startsWith(exclude)) {
                return true;
            }
        }
        // И проверяем с начала строки
        if (normalizedPath.startsWith(exclude)) {
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
