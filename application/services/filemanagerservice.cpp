#include "filemanagerservice.h"
#include <fstream>
#include <algorithm>

namespace application::services {

FileManagerService::FileManagerService(
    std::unique_ptr<domain::services::FilesProc> filesProc,
    const std::string& historyFile)
    : filesProc(std::move(filesProc)), historyFilePath(historyFile) {

    // Подключаем сигналы от сканера
    connect(this->filesProc.get(), &domain::services::FilesProc::scanProgress,
            this, &FileManagerService::onScannerProgress) ;
    loadSearchHistory();
}

void FileManagerService::onScannerProgress(int current, const QString& currentItem) {
    // Просто перенаправляем сигнал в UI
    emit scanProgressUpdated(current, currentItem);
}


void FileManagerService::setDirectoriesConfig(const application::dto::DirsConfig& config) {
    m_dirs_config = config;
}

const application::dto::DirsConfig FileManagerService::getDirectoriesConfig() const {
    return m_dirs_config;
}


// void FileManagerService::setRootDirectories(const ::application::dto::DirsConfig dir_paths) {
//     m_dir_paths = dir_paths;
// }

::application::dto::DirsConfig FileManagerService::normalizeDirsForScanning(
    const ::application::dto::DirsConfig& config) const {

    // Так как в windows слэш может быть как прямой, так и обратный,
    //      преобразуем все слэши в нативные
    //      а также для windows преобразуем всё в low case
    ::application::dto::DirsConfig normalized = config;
#ifdef Q_OS_WIN
    for (QString& dir : normalized.roots) {
        dir = QDir::toNativeSeparators(dir);  // Используем Qt функцию
        dir = dir.toLower();
    }
    for (QString& dir : normalized.exclude) {
        dir = QDir::toNativeSeparators(dir);
        dir = dir.toLower();
    }
#endif

    // Если кто-нибудь вручную отредактирует ini-файл, то в конце корня может не быть слэша
    // Добавляем разделитель в конце корневых путей
    for (QString& dir : normalized.roots) {
        if (!dir.endsWith(QDir::separator())) {
            dir += QDir::separator();
        }
    }

    return normalized;
}

void FileManagerService::scanFileSystem() {
    if (m_dirs_config.roots.empty()) return;

    // Нормализуем конфигурацию для сканирования
    ::application::dto::DirsConfig scan_config = normalizeDirsForScanning(m_dirs_config);

    // FileWorker возвращает данные, мы их сохраняем
    fileSystem = filesProc->scanFileSystem(scan_config);
    searchResultIndices.clear();
}
/*void FileManagerService::scanFileSystem() {
    if (m_dir_paths.roots.empty()) return;

    ::application::dto::DirsConfig dirs_conf_norm { m_dir_paths } ;
#ifdef Q_OS_WIN
    for ( QString & dir : dirs_conf_norm.roots ) {
        dir.replace( '/' , '\\' ) ;
        dir = dir.toLower() ;
    }
    for ( QString & dir : dirs_conf_norm.exclude ) {
        dir.replace( '/' , '\\' ) ;
        dir = dir.toLower() ;
    }
#endif
    for ( QString & dir : dirs_conf_norm.roots )
        if ( ! dir.endsWith( QDir::separator() ) )
            dir += QDir::separator() ;

    // FileWorker возвращает данные, мы их сохраняем
    fileSystem = filesProc->scanFileSystem(dirs_conf_norm);
    searchResultIndices.clear();
}
*/
void FileManagerService::searchFiles(const std::string& searchTerm) {
    if (searchTerm.empty()) {
        searchResultIndices.clear();
        return;
    }

    searchResultIndices = filesProc->searchFiles(fileSystem, QString::fromStdString(searchTerm) ) ;
    saveSearchToHistory(searchTerm);
}

bool FileManagerService::createDirectory(const std::string& path) {
    // Implementation would use repository
    return true;
}

bool FileManagerService::remove(const std::string& path) {
    // Implementation would use repository
    return true;
}

bool FileManagerService::rename(const std::string& oldPath, const std::string& newPath) {
    // Implementation would use repository
    return true;
}

bool FileManagerService::copy(const std::string& source, const std::string& destination) {
    // Implementation would use repository
    return true;
}

QString FileManagerService::getNodeFullPath(int index) const {

        return fileSystem[ index ].getFullPath( fileSystem ) ;
}

void FileManagerService::saveSearchToHistory(const std::string& searchTerm) {
    // Remove if already exists
    searchHistory.erase(
        std::remove(searchHistory.begin(), searchHistory.end(), searchTerm),
        searchHistory.end()
        );

    // Add to front
    searchHistory.insert(searchHistory.begin(), searchTerm);

    // Keep only last 50 searches
    if (searchHistory.size() > 50) {
        searchHistory.resize(50);
    }

    // Save to file
    std::ofstream file(historyFilePath);
    if (file.is_open()) {
        for (const auto& term : searchHistory) {
            file << term << "\n";
        }
    }
}


void FileManagerService::loadSearchHistory() {
    std::ifstream file(historyFilePath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            searchHistory.push_back(line);
        }
    }
}

} // namespace application::services
