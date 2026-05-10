#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include <QTableView>
#include <QLineEdit>
#include "../src/core/config_handler.h"
#include "../src/core/events.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/gui/widgets/secure_table/VaultTableModel.h"
#include "../src/gui/widgets/secure_table/SearchProxyModel.h"
#include "../src/core/vault/VaultManager.h"
#include "../src/gui/widgets/audit_log/audit_logViewer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    ConfigHander &config;
    Database &db;
    VaultManager& m_vaultManager;


    bool isLoggedIn;
    QString m_temporaryMessage;

    QTimer* m_clipboardUpdateTimer;
    int m_clipboardSeconds;

    AuditLogViewer* m_auditLogViewer;

    // Элементы UI
    QMenuBar *menuBar;
    QToolBar *toolBar;
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    //SecureTable *passwordTable;
    QStatusBar *statusBar;

    QTableView* m_tableView;
    VaultTableModel* m_tableModel;
    SearchProxyModel* m_proxyModel;
    QLineEdit* m_searchField;

    // Таймер бездействия
    QTimer *inactivityTimer;
    static const int INACTIVITY_TIMEOUT_MS = 60 * 60 * 1000; // 1 час

    // Методы создания UI
    void createMenuBar();
    void createToolBar();
    void createCentralWidget();
    void createStatusBar();

    // Методы для работы с состоянием
    bool showFirstRunWizard();
    bool showLoginDialog();
    void unlockApplication();
    void lockApplication();
    void resetInactivityTimer();

    // Обработчики событий
    void registerEventHandlers();
    void onUserLoggedIn(const Event& event);
    void onUserLoggedOut(const Event& event);

    void refreshTable();
    void keyPressEvent(QKeyEvent* event) override;

    void showTemporaryMessage(const QString& msg, int timeoutMs = 3000);
    void updatePermanentStatus();

    QTimer* m_clipboardTimer;


private slots:
    void onInactivityTimeout();
    void onLock();

    // Обработчики меню
    void onNewDatabase();
    void onOpenDatabase();
    void onExport();
    void onExit();
    void onAddEntry();
    void onEditEntry();
    void onDeleteEntry();
    void onSettings();
    void onAbout();
    void onFirstRunWizard();
    void onChangePassword();
    void onImport();
    void onShare();
    void onExportPublicKey();
    void onImportPublicKey();

    void showContextMenu(const QPoint& pos);
    void onCopyUsername();
    void onCopyPassword();
    void onCopyAll();

    void onViewAuditLogs();

public:
    MainWindow(ConfigHander &cfg, Database &database, VaultManager& vaultManager);
    ~MainWindow();

    void loadSampleData();
    void updateStatusBar();
};

#endif // MAINWINDOW_H
