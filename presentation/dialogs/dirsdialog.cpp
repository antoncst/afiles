#include "dirsdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidgetItem>

namespace presentation::dialogs {

DirectoriesDialog::DirectoriesDialog(QWidget* parent)
    : QDialog(parent) {

    setupUI();
    setupConnections();
    UpdateDialogState();

    setWindowTitle("Manage Directories");
    setMinimumSize(600, 500);
}

void DirectoriesDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Группа для корневых директорий
    QGroupBox* rootsGroup = new QGroupBox(tr("Папки (будут сканированы)") );
    QVBoxLayout* rootsLayout = new QVBoxLayout(rootsGroup);

    QHBoxLayout* rootsControlLayout = new QHBoxLayout();
    rootsListWidget = new QListWidget();
    rootsListWidget->setSelectionMode(QListWidget::ExtendedSelection);

    addRootButton = new QPushButton("Add...");
    removeRootButton = new QPushButton("Remove");
    moveRootUpButton = new QPushButton("Move Up");
    moveRootDownButton = new QPushButton("Move Down");

    rootsControlLayout->addWidget(addRootButton);
    rootsControlLayout->addWidget(removeRootButton);
    rootsControlLayout->addStretch();
    rootsControlLayout->addWidget(moveRootUpButton);
    rootsControlLayout->addWidget(moveRootDownButton);

    rootsLayout->addWidget(rootsListWidget);
    rootsLayout->addLayout(rootsControlLayout);

    // Группа для исключенных директорий
    QGroupBox* excludeGroup = new QGroupBox(tr("Если в имени папки встретится подстрока, то эта папка НЕ будет сканирована") ) ;
    QVBoxLayout* excludeLayout = new QVBoxLayout(excludeGroup);

    QHBoxLayout* excludeControlLayout = new QHBoxLayout();
    excludeListWidget = new QListWidget();
    excludeListWidget->setSelectionMode(QListWidget::ExtendedSelection);

    addExcludeButton = new QPushButton("Add...");
    removeExcludeButton = new QPushButton("Remove");

    excludeControlLayout->addWidget(addExcludeButton);
    excludeControlLayout->addWidget(removeExcludeButton);
    excludeControlLayout->addStretch();

    excludeLayout->addWidget(excludeListWidget);
    excludeLayout->addLayout(excludeControlLayout);

    label_root_dirs = new QLabel( tr("&Корневые папки \n разделённые ';' :") ) ;
    edit_root_dirs = new QLineEdit() ;
    label_root_dirs->setBuddy( edit_root_dirs ) ;

    hbox_root = new QHBoxLayout() ;
    hbox_root->addWidget( label_root_dirs ) ;
    hbox_root->addWidget( edit_root_dirs ) ;

    label_excl_dirs = new QLabel( tr("Папки, <b>исключаемые</b> из поиска:") ) ;
    label_excl_dirs->setTextFormat(Qt::RichText) ;
    edit_excl_dirs = new QLineEdit() ;
    label_excl_dirs->setBuddy( edit_excl_dirs ) ;

    hbox_excl = new QHBoxLayout() ;
    hbox_excl->addWidget( label_excl_dirs ) ;
    hbox_excl->addWidget( edit_excl_dirs ) ;
    
    // Кнопки диалога
    buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply
        );

    mainLayout->addLayout( hbox_root ) ;
    mainLayout->addWidget(rootsGroup);
    mainLayout->addLayout( hbox_excl );
    mainLayout->addWidget(excludeGroup);
    mainLayout->addWidget(buttonBox);
}

void DirectoriesDialog::setupConnections() {
    connect(addRootButton, &QPushButton::clicked, this, &DirectoriesDialog::onAddRootClicked);
    connect(removeRootButton, &QPushButton::clicked, this, &DirectoriesDialog::onRemoveRootClicked);
    connect(addExcludeButton, &QPushButton::clicked, this, &DirectoriesDialog::onAddExcludeClicked);
    connect(removeExcludeButton, &QPushButton::clicked, this, &DirectoriesDialog::onRemoveExcludeClicked);
    connect(moveRootUpButton, &QPushButton::clicked, this, &DirectoriesDialog::onMoveRootUp);
    connect(moveRootDownButton, &QPushButton::clicked, this, &DirectoriesDialog::onMoveRootDown);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &DirectoriesDialog::UpdateDialogState);

    connect(rootsListWidget, &QListWidget::itemSelectionChanged, this, &DirectoriesDialog::UpdateDialogState);
    connect(excludeListWidget, &QListWidget::itemSelectionChanged, this, &DirectoriesDialog::UpdateDialogState);

    connect( edit_root_dirs , &QLineEdit::editingFinished  ,
            this, &DirectoriesDialog::onRootDirsEditFinished ) ;
    connect( edit_excl_dirs , &QLineEdit::editingFinished  ,
            this, &DirectoriesDialog::onExclDirsEditFinished ) ;


}

QStringList dirs_string_to_list( const QString & qs_dirs )
{
    QStringList qsl_dirs = qs_dirs.split(";" , Qt::SkipEmptyParts ) ; //qt6
    //QStringList dirs_all = qs_dirs.split(";" , QString::SplitBehavior::SkipEmptyParts ) ; //qt5

    for ( int i = qsl_dirs.size() - 1 ; i >= 0  ; --i ) {
        qsl_dirs[i] = qsl_dirs[i].trimmed() ;
        if ( qsl_dirs[i] == "" )
            qsl_dirs.removeAt( i ) ;
    }
    return qsl_dirs ;
}

bool DirectoriesDialog::showValidationWarnings(const application::dto::DirsConfig::ValidationResult& validation)
{
    QString warningText = "Please review the following warnings:\n\n";

    for (const auto& warning : validation.warnings) {
        warningText += "• " + warning + "\n";
    }

    warningText += "\nDo you want to continue anyway?";

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Configuration Warnings");
    msgBox.setText(warningText);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    return msgBox.exec() == QMessageBox::Yes;
}


//         is_ok  not existing dirs
std::pair< bool , QString          > AreDirsExist( std::vector< QString > dirs )
{
    bool is_ok = true ;
    QString notExistingDirs { "" } ;
    for ( const QString & dir : dirs )
        if ( ! QDir(  dir  ).exists() ) {
            if ( ! is_ok ) // значит Не Первый Раз попали в эту ветку
                notExistingDirs += "; " ;
            is_ok = false ;
            notExistingDirs += dir ;
        }
    return {  is_ok , notExistingDirs } ;
}


bool DirectoriesDialog::Validate() {
    //bool is_ok =
    QStringList qsl_dirs_root = dirs_string_to_list( edit_root_dirs->text() ) ;

    std::vector< QString > vec_roots = stringListToVector( qsl_dirs_root ) ;
    if ( vec_roots != dirs_config.roots ) {
        auto are_dirs_exist = AreDirsExist( vec_roots ) ;
        if ( ! are_dirs_exist.first ) {
            QMessageBox::critical( nullptr , QString("title") ,
                                  "Неверный путь: " + are_dirs_exist.second ) ;
            return false;
        }
    }
    auto validation = dirs_config.validate();
    bool result = validation.isValid ;
    if ( ! result ) {
        result = showValidationWarnings( validation ) ;
    }
    return result ;

}

void DirectoriesDialog::accept() /*override*/ {

    if (!Validate()) return;     // Остаемся открытыми
    //if (!confirmWarnings()) return; // Даем выбор пользователю

    //saveState();                 // Сохраняем настройки
    QDialog::accept();           // Закрываем диалог
}




void DirectoriesDialog::onRootDirsEditFinished()
{
    QStringList qsl_dirs_root = dirs_string_to_list( edit_root_dirs->text() ) ;

    std::vector< QString > vec_roots = stringListToVector( qsl_dirs_root ) ;
    if ( vec_roots != dirs_config.roots ) {

        for ( auto & sdir : vec_roots ) {
            // i.e. if root_dir is "c:" or "c:\"
            if ( ! sdir.endsWith( QDir::separator() ) && ! sdir.endsWith( '/' ) )
                sdir = sdir + QDir::separator() ;
        }

        auto are_dirs_exist = AreDirsExist( vec_roots ) ;
        if ( ! are_dirs_exist.first ) {
            QMessageBox::critical( nullptr , QString("title") ,
                                  "Неверный путь: " + are_dirs_exist.second ) ;
            return ;
        }

        dirs_config.roots = vec_roots ;
        updateRootsList();
    }
}

void DirectoriesDialog::onExclDirsEditFinished()
{
    QStringList qsl_dirs_excl = dirs_string_to_list( edit_excl_dirs->text() ) ;

    std::vector< QString > vec = stringListToVector( qsl_dirs_excl ) ;
    if ( vec != dirs_config.exclude ) {
        dirs_config.exclude = vec ;
        updateExcludeList();
    }
}

QString dirs_vec_to_str( std::vector< QString > dirs_vec )
{
    QString result = "" ;
    for ( QString & dir : dirs_vec )
        result += dir + "; " ;
    return result ;
}

void DirectoriesDialog::updateRootsEdit()
{
    edit_root_dirs->setText( dirs_vec_to_str( dirs_config.roots ) );
}

void DirectoriesDialog::updateExcludeEdit()
{
    edit_excl_dirs->setText( dirs_vec_to_str( dirs_config.exclude ) );
}

void DirectoriesDialog::setConfig(const application::dto::DirsConfig& config) {
    this->dirs_config = config;
    updateRootsList();
    updateRootsEdit();
    updateExcludeList();
    updateExcludeEdit();
}

application::dto::DirsConfig DirectoriesDialog::getConfig() const {
    return dirs_config;
}

application::dto::DirsConfig DirectoriesDialog::getDirsConfig(
    const application::dto::DirsConfig& initial, QWidget* parent) {

    DirectoriesDialog dialog(parent);
    dialog.setConfig(initial);

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.getConfig();
    }

    return initial; // Возвращаем исходную конфигурацию если отменили
}

void DirectoriesDialog::onAddRootClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Root Directory",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (!dir.isEmpty()) {
        dirs_config.addRoot(dir);
        updateRootsList();
        updateRootsEdit();
    }
}

void DirectoriesDialog::onRemoveRootClicked() {
    QList<QListWidgetItem*> selected = rootsListWidget->selectedItems();
    if (selected.isEmpty()) return;

    for (QListWidgetItem* item : selected) {
        QString path = item->text() ;
        dirs_config.removeRoot(path);
    }
    updateRootsList();
    updateRootsEdit();
}

void DirectoriesDialog::onAddExcludeClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Directory to Exclude",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (!dir.isEmpty()) {
        dirs_config.addExclude(dir);
        updateExcludeList();
        updateExcludeEdit();
    }
}

void DirectoriesDialog::onRemoveExcludeClicked() {
    QList<QListWidgetItem*> selected = excludeListWidget->selectedItems();
    if (selected.isEmpty()) return;

    for (QListWidgetItem* item : selected) {
        QString path = item->text() ;
        dirs_config.removeExclude(path);
    }
    updateExcludeList();
    updateExcludeEdit();
}

void DirectoriesDialog::onMoveRootUp() {
    int currentRow = rootsListWidget->currentRow();
    if (currentRow > 0) {
        // Меняем местами в конфиге
        std::swap(dirs_config.roots[currentRow], dirs_config.roots[currentRow - 1]);
        updateRootsList();
        updateRootsEdit();
        rootsListWidget->setCurrentRow(currentRow - 1);
    }
}

void DirectoriesDialog::onMoveRootDown() {
    int currentRow = rootsListWidget->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(dirs_config.roots.size()) - 1) {
        // Меняем местами в конфиге
        std::swap(dirs_config.roots[currentRow], dirs_config.roots[currentRow + 1]);
        updateRootsList();
        updateRootsEdit();
        rootsListWidget->setCurrentRow(currentRow + 1);
    }
}

void DirectoriesDialog::UpdateDialogState() {
    // Обновляем состояние кнопок
    bool hasRootSelection = !rootsListWidget->selectedItems().isEmpty();
    bool hasExcludeSelection = !excludeListWidget->selectedItems().isEmpty();
    int rootCurrentRow = rootsListWidget->currentRow();

    removeRootButton->setEnabled(hasRootSelection);
    removeExcludeButton->setEnabled(hasExcludeSelection);
    moveRootUpButton->setEnabled(hasRootSelection && rootCurrentRow > 0);
    moveRootDownButton->setEnabled(hasRootSelection &&
                                   rootCurrentRow < static_cast<int>(dirs_config.roots.size()) - 1);

    // Проверяем что есть хотя бы одна корневая директория
    bool isNotEmpty = !dirs_config.roots.empty();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isNotEmpty);

    if (!isNotEmpty) {
        setStyleSheet("QGroupBox { color: #CC9900; }");
    } else {
        setStyleSheet("");
    }
}

void DirectoriesDialog::updateRootsList() {
    rootsListWidget->clear();
    for (const auto& root : dirs_config.roots) {
        rootsListWidget->addItem(root);
    }
    UpdateDialogState();
}

void DirectoriesDialog::updateExcludeList() {
    excludeListWidget->clear();
    for (const auto& exclude : dirs_config.exclude) {
        excludeListWidget->addItem(exclude);
    }
    UpdateDialogState();
}

QStringList DirectoriesDialog::vectorToStringList(const std::vector<std::string>& vec) const {
    QStringList list;
    for (const auto& item : vec) {
        list.append(QString::fromStdString(item));
    }
    return list;
}

std::vector<QString> DirectoriesDialog::stringListToVector(const QStringList& list) const {
    std::vector<QString> vec;
    for (const QString& item : list) {
        vec.push_back(item);
    }
    return vec;
}

} // namespace presentation::dialogs
