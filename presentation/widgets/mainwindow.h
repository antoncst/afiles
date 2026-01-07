#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QListView>
#include <QLineEdit>
#include <QCheckBox>
#include <QTreeView>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringListModel>
#include <QProgressDialog>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMutex>
#include <QThread>

#include "../../application/services/filemanagerservice.h"
#include "../../infrastructure/logging/logger.h"
#include "../../presentation/workers/filescanworker.h"
#include "../../domain/utils/historynavigator.hpp"

namespace presentation::widgets {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(
        std::shared_ptr<application::services::FileManagerService> fileService,
        QWidget* parent = nullptr
        );
    ~MainWindow();

    static constexpr char qs_ini_filename[] = "./FileSearchSet.ini" ;
    static constexpr qsizetype max_query_compelete_count = 760 ;
    static constexpr size_t max_query_history_count = 380 ;
    static constexpr size_t max_dirs_history_count = 40 ;

private slots:

    void on_query_edit( const QString & ) ; // lineedit_query->textEdited
    void on_query_returnpressed( ) ; // lineedit_query->returnPressed()
    void on_query_editfinished() ;
    //void on_dirsedit_finished() ; // lineedit_dirs ->editingFinished
    // void on_hidden_files_checked() ; // chkbx_hiddenFiles ->stateChanged
    void on_history_go_back() ; // button_hist_left ->clicked , action alt-left
    void on_history_go_forward() ; // button_hist_right ->clicked
    // void slot_run_action() ; // run_action ->triggered
    // void slot_edit_action() ; // edit_action ->triggered
    // void slot_openfolder_action() ;
    // void item_activated( const QModelIndex & ) ; // searchResultsView ->activated
    // void item_doubleClicked(const QModelIndex & ) ;
    void add_query_into_complete_list() ;
    //void start_build_tree() ;
    void set_history_bttns_visibility() ;

    //void on_tree_built() ;

    //слоты для file_scan_worker
    void onScanFinished();
    void onScanError(const QString& error);
    //void onScanProgress(int value, const QString& message);



    // void onScanClicked();
    // void onRemoveRootClicked();
     void onSearchClicked();
    // void onSearchTextChanged(const QString& text);
    // void onSearchHistorySelected(const QString& searchTerm);
    // void onFileContextMenuRequested(const QPoint& pos);
    // void onScanFinished();
    // void onScanProgress(int current, int total);

    void createNewDirectory();
    void deleteSelectedFile();
    void renameSelectedFile();
    void copySelectedFile();

    void onManageDirectoriesClicked();
    void onScanProgressUpdated(int current, const QString& currentItem) ;
private:

    std::shared_ptr<application::services::FileManagerService> fileService { nullptr } ;

    void setupUI();
    void SetUIEnablesOnScan(bool enabled) ;
    void setupMenuBar();
    //void setupToolBar();
    void setupConnections();
    void updateSearchResultsView();
    void updateSearchHistoryCombo() ;
    void showError(const QString& message);
    void showInfo(const QString& message);
    void scanFileSystem();
    void cleanupThread();
    bool ckeckupIsThreadClean() ;

    //void updateDirectoriesConfig();

    // UI Components
    QWidget* centralWidget;
    // QVBoxLayout* mainLayout;
    // QHBoxLayout* controlLayout;
    // QHBoxLayout* rootsLayout;

    // QLineEdit* searchEdit;
    // QComboBox* searchHistoryCombo;
    // QPushButton* scanButton;
    // QPushButton* searchButton;
    // QPushButton* addRootButton;
    // QPushButton* removeRootButton;

    QListView* searchResultsView;
    // QListView* rootsListView;
    QStringListModel* searchResultsModel;
    // QStringListModel* rootsListModel;

    QMenuBar* menuBar;
    // QToolBar* toolBar;
     QMenu* fileMenu;
    // QMenu* editMenu;
    // QMenu* viewMenu;

    // QStatusBar* statusBar;

    // QProgressDialog* progressDialog;
    // QTimer* progressTimer;
    // int scanProgressValue = 0;


    QVBoxLayout* mainLayout ;
    QLabel * label_root_dirs_title ;
    QLabel * label_root_dirs ;
    QPushButton* button_conf_dirs ;
    QLabel * label_query ;
    QLineEdit * lineedit_query ;
    QLabel * label_hiddenFiles ;
    QCheckBox * chkbx_hiddenFiles ;
    QPushButton * button_stop ;
    QPushButton * button_hist_left ;
    QPushButton * button_hist_right ;

    QHBoxLayout * hbox1 ;
    QHBoxLayout * hbox2 ;
    QVBoxLayout * vbox ;

    QStatusBar * statusBar ;
    QAction * run_action ;
    QAction * go_back_action ;
    QAction * go_forward_action ;
    QAction * edit_action ;
    QAction * openfolder_action ;
    QCompleter * query_completer = nullptr ;
    QStringList * query_stringlist ;
    QSystemTrayIcon * trayIcon;
    QMenu * trayIconMenu;

    QAction * m_manageDirectoriesAction ;
    QMutex * pmutex ;

    historyNavigator< QString > query_history ;

    // Qt Threading
    QThread* workerThread = nullptr ;
    workers::FileScanWorker* file_scan_worker;

    bool just_built_tree = false ; // for statusBar adding message (do not clear current message)
    bool needToSaveSettings = false ;
    void writeSettings() ;
    void readSettings() ;

    int timer_id ;
protected:
/*    void showEvent(QShowEvent *ev) override
    {
        QWidget::showEvent(ev);
        // Call slot via queued connection so it's called from the UI thread after this method has returned and the window has been shown
        QMetaObject::invokeMethod(this, "afterWindowShown", Qt::ConnectionType::QueuedConnection);
    }
*/
    void resizeEvent( QResizeEvent * ) override ;
    void moveEvent( QMoveEvent * ) override ;
    //void timerEvent(QTimerEvent *) override ;
    bool eventFilter(QObject *target, QEvent *event) override ;

public slots:
    void stop_scan_files() {
            fileService->stopScanFiles() ; }

};

} // namespace presentation::widgets

#endif // MAINWINDOW_H
