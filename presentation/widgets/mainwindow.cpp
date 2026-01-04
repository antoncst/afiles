#include "mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QProgressDialog>
#include <QTimer>
#include <QThread>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>

//#include <thread>
#include <chrono>

//#include "../../domain/dirspaths.h"
//#include "../dirshelper.h"
#include "../dialogs/dirsdialog.h"

QString formatNumber( intmax_t number )
{
    QLocale locale = QLocale::system();
    return locale.toString(number);
}

namespace presentation::widgets {

MainWindow::MainWindow(
    std::shared_ptr<application::services::FileManagerService> fileService,
    QWidget* parent)
    : QMainWindow(parent), fileService(fileService) {

    setupUI();
    setupMenuBar();
    //setupToolBar();
/*
    // Инициализируем прогресс-диалог
    progressDialog = new QProgressDialog("Scanning file system...", "Cancel", 0, 100, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setAutoClose(true);
    progressDialog->setAutoReset(true);

    progressTimer = new QTimer(this);
*/
    setupConnections();
    readSettings();

    scanFileSystem(); ;


}

MainWindow::~MainWindow() {
    cleanupThread();
}

bool MainWindow::ckeckupIsThreadClean() {
    if (workerThread)
        if (workerThread->isRunning())
            return false ;
    return true ;
}

void MainWindow::cleanupThread() {
    if (workerThread) {
        qDebug() << "cleaning up thread" ;
        if (workerThread->isRunning()) {
            fileService->stopScanFiles() ;
            workerThread->wait(200); // Ждем 0.3 секунды для graceful shutdown
            workerThread->quit();
            QThread::msleep(50) ;
            fileService->resetStopState() ;
        }
        delete workerThread;
        workerThread = nullptr;
    }

    // Worker удалится автоматически при удалении thread
    file_scan_worker = nullptr;
}


void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);
/*
    // Панель управления корневыми директориями
    rootsLayout = new QHBoxLayout();

    QLabel* rootsLabel = new QLabel("Root Directories:");
    rootsListView = new QListView();
    rootsListModel = new QStringListModel(this);
    rootsListView->setModel(rootsListModel);
    rootsListView->setSelectionMode(QListView::SingleSelection);

    addRootButton = new QPushButton("Add Root");
    removeRootButton = new QPushButton("Remove Root");

    rootsLayout->addWidget(rootsLabel);
    rootsLayout->addWidget(rootsListView, 1);
    rootsLayout->addWidget(addRootButton);
    rootsLayout->addWidget(removeRootButton);

    // Панель управления поиском
    controlLayout = new QHBoxLayout();

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Enter search term...");

    searchHistoryCombo = new QComboBox();
    searchHistoryCombo->setEditable(true);
    searchHistoryCombo->setMinimumWidth(150);

    scanButton = new QPushButton("Scan File System");
    scanButton->setMinimumWidth(120);

    searchButton = new QPushButton("Search");
    searchButton->setMinimumWidth(80);
    searchButton->setEnabled(false);

    controlLayout->addWidget(new QLabel("Search:"));
    controlLayout->addWidget(searchEdit, 2);
    controlLayout->addWidget(new QLabel("History:"));
    controlLayout->addWidget(searchHistoryCombo, 1);
    controlLayout->addWidget(scanButton);
    controlLayout->addWidget(searchButton);
*/
    // Результаты поиска
    searchResultsView = new QListView();
    //searchResultsModel = new QStringListModel(this);
    //searchResultsView->setModel(searchResultsModel);
    searchResultsView->setContextMenuPolicy(Qt::CustomContextMenu);
/*
    // Основной layout
    mainLayout->addLayout(rootsLayout);
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(new QLabel("Search Results:"));
    mainLayout->addWidget(searchResultsView, 1);

    // Status bar
    statusBar = new QStatusBar();
    setStatusBar(statusBar);
    statusBar->showMessage("Ready to scan file system");

    // Обновляем историю поиска
    //updateSearchHistoryCombo();
*/
    trayIcon = new QSystemTrayIcon ;
    trayIcon->setVisible( true ) ;

    label_query = new QLabel( tr("&Запрос:") ) ;
    lineedit_query = new QLineEdit() ;
    label_query->setBuddy( lineedit_query ) ;

    label_hiddenFiles = new QLabel( tr("Скрытые") ) ;
    chkbx_hiddenFiles = new QCheckBox() ;
    chkbx_hiddenFiles->setFocusPolicy( Qt::NoFocus ) ;
    label_hiddenFiles->setBuddy( chkbx_hiddenFiles ) ;

    button_stop = new QPushButton( tr( "Стоп" ) ) ;
    button_stop->setFocusPolicy( Qt::NoFocus ) ;
    button_stop->setVisible( false ) ;

    button_hist_left = new QPushButton( "◀" ) ;
    button_hist_left->setToolTip( tr( "Назад (Alt-Left)" ) ) ;
    button_hist_left->setMaximumWidth( 20 ) ;
    button_hist_left->setFocusPolicy( Qt::NoFocus ) ;

    button_hist_right = new QPushButton( "▶" ) ;
    button_hist_right->setToolTip( tr( "Вперед (Alt-Right)" ) ) ;
    button_hist_right->setMaximumWidth( 20 ) ;
    button_hist_right->setFocusPolicy( Qt::NoFocus ) ;

    hbox = new QHBoxLayout() ;
    hbox->addWidget( label_query ) ;
    hbox->addWidget( lineedit_query ) ;
    hbox->addWidget( label_hiddenFiles ) ;
    hbox->addWidget( chkbx_hiddenFiles ) ;
    hbox->addWidget( button_stop ) ;
    hbox->addWidget( button_hist_left ) ;
    hbox->addWidget( button_hist_right ) ;

    statusBar = new QStatusBar() ;
    searchResultsModel = new QStringListModel(this);
    searchResultsView = new QListView() ;

    mainLayout->addLayout( hbox ) ;
    mainLayout->addWidget( searchResultsView ) ;
    mainLayout->addWidget( statusBar ) ;

}

void MainWindow::setupMenuBar() {
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    fileMenu = menuBar->addMenu("File");
    fileMenu->addAction(tr("Корневые папки..."), this, &MainWindow::onManageDirectoriesClicked );
    //fileMenu->addAction("Scan File System", this, &MainWindow::onScanClicked);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Выход"), this, &QWidget::close);

    // editMenu = menuBar->addMenu("Edit");
    // editMenu->addAction("New Directory", this, &MainWindow::createNewDirectory);
    // editMenu->addAction("Delete", this, &MainWindow::deleteSelectedFile);
    // editMenu->addAction("Rename", this, &MainWindow::renameSelectedFile);
    // editMenu->addAction("Copy", this, &MainWindow::copySelectedFile);

    // viewMenu = menuBar->addMenu("View");
    // viewMenu->addAction("Refresh", this, &MainWindow::onScanClicked);
}

/*
void MainWindow::setupToolBar() {
    toolBar = addToolBar("Main Toolbar");
    toolBar->addAction(QIcon(":/icons/add.png"), "Add Root", this, &MainWindow::onManageDirsClicked);
    toolBar->addAction(QIcon(":/icons/scan.png"), "Scan", this, &MainWindow::onScanClicked);
    toolBar->addAction(QIcon(":/icons/search.png"), "Search", this, &MainWindow::onSearchClicked);
    toolBar->addSeparator();
    toolBar->addAction(QIcon(":/icons/refresh.png"), "Refresh", this, &MainWindow::onScanClicked);
}
*/
void MainWindow::setupConnections() {
/*
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(addRootButton, &QPushButton::clicked, this, &MainWindow::onAddRootClicked);
    connect(removeRootButton, &QPushButton::clicked, this, &MainWindow::onRemoveRootClicked);
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(searchHistoryCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onSearchHistorySelected);
    connect(searchResultsView, &QListView::customContextMenuRequested,
            this, &MainWindow::onFileContextMenuRequested);

    connect(progressDialog, &QProgressDialog::canceled, this, [this]() {
        progressTimer->stop();
        statusBar->showMessage("Scan cancelled");
    });

    connect(progressTimer, &QTimer::timeout, this, [this]() {
        scanProgressValue = (scanProgressValue + 5) % 100;
        progressDialog->setValue(scanProgressValue);
    });
*/
    lineedit_query->installEventFilter( this ) ;
    connect( lineedit_query  , &QLineEdit::textEdited ,
            this, &MainWindow::on_query_edit ) ; //( const QString & )
    connect( lineedit_query  , &QLineEdit::returnPressed ,
            this, &MainWindow::on_query_returnpressed ) ; //( const QString & )
    connect( lineedit_query  , &QLineEdit::editingFinished  ,
            this, &MainWindow::on_query_editfinished ) ;
    connect( button_stop , &QPushButton::clicked  , this , &MainWindow::stop_scan_files ) ;
    connect(fileService.get(), &application::services::FileManagerService::scanProgressUpdated,
            this, &MainWindow::onScanProgressUpdated);

    // connect ( chkbx_hiddenFiles , &QCheckBox::stateChanged ,// (int state)
    //         this , &MainWindow::on_hidden_files_checked ) ;
    // connect( button_hist_left , &QPushButton::clicked  ,
    //         this , &MainWindow::on_history_go_back ) ;
    // connect( button_hist_right , &QPushButton::clicked  ,
    //         this , &MainWindow::on_history_go_forward ) ;

    /*connect(progressDialog, &QProgressDialog::canceled, this, [this]() {
        if (workerThread && workerThread->isRunning()) {
            workerThread->requestInterruption();
            workerThread->quit();
        }
        statusBar->showMessage("Scan cancelled by user");
    });*/

    run_action = new QAction(tr("Открыть файл (Enter)") , this) ; // Run
    run_action->setShortcut( QKeySequence( Qt::ControlModifier | Qt::Key_R ) );
    run_action->setStatusTip(tr("Запустить") ) ;
    //connect( run_action , &QAction::triggered  , this , &MainWindow::slot_run_action  ) ;

    edit_action = new QAction(tr("Режим редактирования") , this) ;
    edit_action->setShortcut( QKeySequence(  Qt::Key_F2 ) );
    edit_action->setStatusTip(tr("Для копирования текста, не для исправления") ) ;
    //connect( edit_action , &QAction::triggered  , this , &MainWindow::slot_edit_action  ) ;

    openfolder_action = new QAction( tr("Открыть папку"), this ) ;
    //connect( openfolder_action , &QAction::triggered  , this , &MainWindow::slot_openfolder_action  ) ;

    query_stringlist = new QStringList() ;

    searchResultsView->addAction( run_action ) ;
    searchResultsView->addAction( edit_action ) ;
    searchResultsView->addAction(  openfolder_action );

    searchResultsView->setContextMenuPolicy( Qt::ActionsContextMenu ) ;

    go_back_action = new QAction(tr("Back") , this) ;
    go_back_action->setShortcut( QKeySequence( Qt::AltModifier | Qt::Key_Left ) );
    //go_back_action->setStatusTip(tr("") ) ;
    //connect( go_back_action , &QAction::triggered  , this , &MainWindow::on_history_go_back  ) ;

    go_forward_action = new QAction(tr("Forward") , this) ;
    go_forward_action ->setShortcut( QKeySequence( Qt::AltModifier | Qt::Key_Right ) );
    //go_back_action->setStatusTip(tr("") ) ;
    //connect( go_forward_action  , &QAction::triggered  , this , &MainWindow::on_history_go_forward  ) ;

    addAction( go_back_action ) ;
    addAction( go_forward_action ) ;

    //connect( searchresultsView , &QListView::activated, this, &MainWindow::item_activated ) ;
    //connect( searchresultsView , &QListView::doubleClicked , this, &MainWindow::item_doubleClicked ) ;

    searchResultsModel = new QStringListModel(this);

    timer_id = startTimer( 1000 ) ; // for writeSettings end statusBar

    //query_history.set_max_size( max_query_history_count ) ;
    //dirs_history.set_max_size( max_dirs_history_count );
    //readSettings() ;

    //set_history_bttns_visibility();

}

/* void MainWindow::onManageDirsClicked() {



    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Root Directory",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (!dir.isEmpty()) {
        QStringList currentRoots = rootsListModel->stringList();
        if (!currentRoots.contains(dir)) {
            currentRoots.append(dir);
            rootsListModel->setStringList(currentRoots);
            statusBar->showMessage(QString("Added root directory: %1").arg(dir), 3000);
        } else {
            statusBar->showMessage("Directory already in roots list", 3000);
        }
    }
}*/
/*
void MainWindow::onRemoveRootClicked() {
    QModelIndexList selected = rootsListView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select a root directory to remove.");
        return;
    }

    QStringList currentRoots = rootsListModel->stringList();
    for (const QModelIndex& index : selected) {
        if (index.isValid()) {
            QString dir = currentRoots.at(index.row());
            currentRoots.removeAt(index.row());
            statusBar->showMessage(QString("Removed root directory: %1").arg(dir), 3000);
        }
    }
    rootsListModel->setStringList(currentRoots);
}
*/
/*
void MainWindow::updateDirectoriesConfig() {
    auto config = fileService->getDirectoriesConfig();

    // Обновляем список в UI если нужно
    QStringList rootsList;
    for (const auto& root : config.roots) {
        rootsList.append(QString::fromStdString(root));
    }

    // Можно показать краткую информацию в status bar
    statusBar->showMessage(
        QString("Roots: %1, Excluded: %2").arg(config.roots.size()).arg(config.exclude.size()),
        3000);
}
*/
void MainWindow::onManageDirectoriesClicked() {
    // Получаем текущую конфигурацию
    auto currentConfig = fileService->getDirectoriesConfig();

    // Открываем диалог
    ::application::dto::DirsConfig newConfig = presentation::dialogs::DirectoriesDialog::getDirsConfig(
        currentConfig, this);

    if ( newConfig != currentConfig ) {
        // Применяем новую конфигурацию
        fileService->setDirectoriesConfig(newConfig);

        writeSettings();

        scanFileSystem();

        // Обновляем UI
        //updateDirectoriesConfig();

        // Показываем статистику
        QString message = QString("Configuration updated: %1 root directories, %2 excluded directories")
                              .arg(newConfig.roots.size())
                              .arg(newConfig.exclude.size());
        showInfo(message);
    }
}


void MainWindow::onScanProgressUpdated(int current, const QString& currentItem) {
    // Обновляем статус бар каждые 100мс
    QString status = QString("Найдено: %1 элементов | Текущий: %2")
                         .arg(formatNumber( current))
                         .arg(currentItem);

    statusBar->showMessage(status) ;
}



void MainWindow::scanFileSystem() {

    button_stop->setVisible( true ) ;
    // Получаем список корневых директорий
    //   QStringList roots = rootsListModel->stringList();
    const ::application::dto::DirsConfig dirs_conf = fileService->getDirectoriesConfig() ;
    if ( dirs_conf.roots.empty() ) {
        showError("No root directories selected");
        return;
    }

    if ( !ckeckupIsThreadClean() ) {
        showError(" Поток сканирования дерева файлов ещё работает..." ) ;
        return ;
    }

    // Очищаем предыдущий thread если есть
    cleanupThread();

    // Настраиваем UI для сканирования
    // scanButton->setEnabled(false);
    // searchButton->setEnabled(false);
    // progressDialog->setValue(0);
    // progressDialog->setLabelText("Preparing to scan file system...");
    // progressDialog->show();

    statusBar->showMessage("Starting file system scan...");

    // Создаем и настраиваем file_scan_worker и thread
    workerThread = new QThread(this);

    file_scan_worker = new workers::FileScanWorker(fileService, dirs_conf );

    // Перемещаем file_scan_worker в thread
    file_scan_worker->moveToThread(workerThread);

    // Подключаем сигналы и слоты
    connect(workerThread, &QThread::started, file_scan_worker, &workers::FileScanWorker::process);
    connect(file_scan_worker, &workers::FileScanWorker::finished, this, &MainWindow::onScanFinished);
    connect(file_scan_worker, &workers::FileScanWorker::error, this, &MainWindow::onScanError);
    //connect(file_scan_worker, &workers::FileScanWorker::progress, this, &MainWindow::onScanProgress);

    connect(file_scan_worker, &workers::FileScanWorker::finished, workerThread, &QThread::quit);
    connect(file_scan_worker, &workers::FileScanWorker::finished, file_scan_worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Запускаем thread
    workerThread->start();
}

void MainWindow::onScanFinished() {
    qDebug() << "onScanFinished" ;

    button_stop->setVisible( false ) ;

    const auto& fileSystem = fileService->getFileSystem();
    fileService->resetStopState() ;

    // Показываем статистику
    int totalFiles = 0;
    int totalDirs = 0;

    for (const auto& node : fileSystem) {
        if (node.isDirectory()) {
            totalDirs++;
        } else {
            totalFiles++;
        }
    }

    QString message = QString("Найдено: %1 файлов, %2 папок, %3 всего")
                          .arg(formatNumber( totalFiles)).arg(formatNumber( totalDirs))
                          .arg(formatNumber( fileSystem.size()) );

    showInfo(message);

    // Если был активен поиск - обновляем результаты
    // if (!searchEdit->text().trimmed().isEmpty()) {
    //     onSearchClicked();
    // }

    // Очищаем ресурсы
    cleanupThread();
}

void MainWindow::onScanError(const QString& error) {
    // progressDialog->hide();
    // scanButton->setEnabled(true);
    // searchButton->setEnabled(!searchEdit->text().trimmed().isEmpty());

    showError(error);
    statusBar->showMessage("Scan failed", 5000);

    // Очищаем ресурсы
    cleanupThread();
}


// void MainWindow::onScanProgress(int current, int total) {
//     // В реальном приложении FileWorker бы эмитил сигналы прогресса
//     // Здесь просто анимация
//     if (total > 0) {
//         int progress = (current * 100) / total;
//         progressDialog->setValue(progress);
//     }
// }

void MainWindow::readSettings()
{
    QRect rect_widget ;
    QSettings settings( "afileset.ini" ,  QSettings::IniFormat);
    if ( settings.contains( "geometry" ) )
    {
        rect_widget = settings.value("geometry").toRect();
//костыль местоположения для windows
#ifdef Q_OS_WIN
        int n_amendment = 30 ;
        rect_widget.setTop( rect_widget.top() - n_amendment );
        rect_widget.setHeight(rect_widget.height() - n_amendment ) ;
#endif
    } else
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect rect_screen = screen->availableGeometry() ;
        rect_widget.setTop( rect_screen.height() / 10 ) ;
        rect_widget.setLeft( rect_screen.right() / 5 ); ;
        rect_widget.setWidth( rect_screen.width() / 2 ) ;
        rect_widget.setHeight( rect_screen.height() / 5 * 4 );
    }
    move(rect_widget.topLeft());
    resize(rect_widget.size());

    ::application::dto::DirsConfig dirs_conf ;
    if ( settings.contains( "rootDirs" ) )
    {
        QStringList qlist = settings.value( "rootDirs" ).toStringList() ;
        dirs_conf.roots.reserve( qlist.size() );
        for ( const auto & qs : qlist )
            dirs_conf.roots.emplace_back( qs ) ;
    }

    if ( settings.contains( "excludeDirs" ) )
    {
        QStringList qlist = settings.value( "excludeDirs" ).toStringList() ;
        dirs_conf.exclude.reserve( qlist.size() );
        for ( const auto & qs : qlist )
            dirs_conf.exclude.emplace_back( qs ) ;
    }
#ifdef Q_OS_WIN
    if ( dirs_conf.roots.size() == 0 ) {
        dirs_conf.roots.emplace_back( "c:\\Users" ) ;
        if ( dirs_conf.exclude.size() == 0 )
            dirs_conf.exclude = {
                 {":\\Windows"} ,
                 {":\\Program Files"} ,
                 {"\\AppData\\"} ,
                 {":\\ProgramData"} ,
                 {":\\Recovery"}
            } ;
    }
#endif
    fileService->setDirectoriesConfig( dirs_conf ) ;
}

void MainWindow::writeSettings()
{

    qDebug() << "Saving settings" ;
    QSettings settings( "afileset.ini" , QSettings::IniFormat ) ;
    settings.setValue( "geometry" , geometry() ) ;

    ::application::dto::DirsConfig dirs_conf = fileService->getDirectoriesConfig() ;
    QStringList qslist ;
    for (const auto &s : dirs_conf.roots)
        qslist.append( s );
    settings.setValue( "rootDirs" , qslist ) ;

    qslist.clear();
    for (const auto &s : dirs_conf.exclude)
        qslist.append( s );
    settings.setValue( "excludeDirs" , qslist ) ;

    needToSaveSettings = false ;
}

void MainWindow::on_query_editfinished()
{
    // emit lineedit_query->textEdited( lineedit_query->text() ) ;
    // if ( lineedit_query->text().size() >= 2 )
    // {
    //     query_history.add( lineedit_query->text() ) ;
    //     set_history_bttns_visibility() ;
    //     needToSaveSettings = true ;
    // }
}


void MainWindow::on_query_returnpressed() {
    //emit lineedit_query->textEdited( lineedit_query->text() ) ;
    // after returnPressed event, editingFinished event appears , so nothing to do
}

void MainWindow::on_query_edit( const QString & text ) {

    onSearchClicked() ;
    return ;
/*
    if ( text.size() >= 2 )
    {
        searchresultsView->reset() ;

        std::vector< const tfile * > res_vec =  trees_->find_str( text ) ;

        statusBar->setPalette( QGuiApplication::palette() );
        QString msg = "" ;
        if ( just_built_tree ) {
            just_built_tree = false ;
            msg = statusBar->currentMessage() + " " ;
        }
        msg += tr("Результатов: ") + QString::number( res_vec.size() ) ;
        statusBar->showMessage( msg ) ;
        qDebug() << "Found " << res_vec.size() << " items" ;

        QStringList res_sl ;
        for ( const tfile * file : res_vec )
            res_sl.append( trees_->get_full_name( file ) ) ;  //todo get_full_name()


        searchResultsModel->setStringList( res_sl ) ;

        searchresultsView->setModel(searchResultsModel) ;
    }
*/
}

void MainWindow::onSearchClicked() {
    QString searchTerm = lineedit_query->text().trimmed();
    if (searchTerm.size() < 3 )
        return;

    try {
        fileService->searchFiles(searchTerm.toStdString());
        updateSearchResultsView();
        updateSearchHistoryCombo();

        const auto& results = fileService->getSearchResultIndices();
        showInfo(QString("Found %1 results").arg(results.size()));

    } catch (const std::exception& e) {
        showError(QString("Search failed: %1").arg(e.what()));
    }
}
/*
void MainWindow::onSearchTextChanged(const QString& text) {
    searchButton->setEnabled(!text.trimmed().isEmpty());
}

void MainWindow::onSearchHistorySelected(const QString& searchTerm) {
    searchEdit->setText(searchTerm);
}

void MainWindow::onFileContextMenuRequested(const QPoint& pos) {
    QMenu contextMenu(this);

    contextMenu.addAction("Open Location", this, [this]() {
        // Реализация открытия расположения файла
    });

    contextMenu.addSeparator();
    contextMenu.addAction("New Directory", this, &MainWindow::createNewDirectory);
    contextMenu.addAction("Delete", this, &MainWindow::deleteSelectedFile);
    contextMenu.addAction("Rename", this, &MainWindow::renameSelectedFile);
    contextMenu.addAction("Copy", this, &MainWindow::copySelectedFile);

    contextMenu.exec(searchResultsView->mapToGlobal(pos));
}
*/
void MainWindow::createNewDirectory() {
}
/*
    bool ok;
    QString dirName = QInputDialog::getText(this, "New Directory",
                                            "Directory name:", QLineEdit::Normal, "", &ok);
    if (ok && !dirName.isEmpty()) {
        // Implementation
        showInfo("Directory created");
    }
}
*/
void MainWindow::deleteSelectedFile() {
}
/*
    QModelIndex index = searchResultsView->currentIndex();
    if (!index.isValid()) return;

    // Implementation
    showInfo("File deleted");
}
*/
void MainWindow::renameSelectedFile() {}/*
    QModelIndex index = searchResultsView->currentIndex();
    if (!index.isValid()) return;

    // Implementation
    showInfo("File renamed");
}
*/
void MainWindow::copySelectedFile() {}/*
    QModelIndex index = searchResultsView->currentIndex();
    if (!index.isValid()) return;

    // Implementation
    showInfo("File copied");
}
*/
void MainWindow::updateSearchResultsView() {
    QStringList results;
    const auto& resultIndices = fileService->getSearchResultIndices();

    for (int index : resultIndices) {
        QString fullPath = fileService->getNodeFullPath(index);
        results.append( fullPath );
    }

    searchResultsModel->setStringList(results);
    searchResultsView->setModel( searchResultsModel ) ;
}
/*
void MainWindow::updateRootDirectoriesView() {
    // Метод для обновления списка корневых директорий
    // (уже реализован через rootsListModel)
}
*/
void MainWindow::updateSearchHistoryCombo() {} /*
    searchHistoryCombo->clear();
    for (const auto& term : fileService->getSearchHistory()) {
        searchHistoryCombo->addItem(QString::fromStdString(term));
    }
}
*/
void MainWindow::showError(const QString& message) {
    QMessageBox::critical(this, "Error", message);
    statusBar->showMessage(message, 5000);
}

void MainWindow::showInfo(const QString& message) {
    statusBar->showMessage(message);
}


void MainWindow::resizeEvent(QResizeEvent * event)
{
    //qDebug() << "resizeEvent" << event->size().width() << event->size().height();
    //needToSaveSettings = true ;
    QWidget::resizeEvent(event);
}


void MainWindow::moveEvent( QMoveEvent * event )
{
    //needToSaveSettings = true ;
    QWidget::moveEvent(event);
}


bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
/*    if ( target == lineedit_query )
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent * keyEvent = static_cast< QKeyEvent * >( event) ;
            if ( keyEvent->key() == Qt::Key_Down )
            {
                searchresultsView->setFocus() ;
                return true;
            }
        }
    }*/
    return QWidget::eventFilter( target, event ) ;
}

} //namespace presentation::widgets
