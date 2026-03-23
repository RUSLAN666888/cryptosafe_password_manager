#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>
#include "../src/core/config_handler.h"
#include "../src/core/events.h"
#include "../src/core/key_manager.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/gui/widgets/secure_table/SecureTable.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    ConfigHander &config;
    Database &db;
    bool isLoggedIn;

    // Элементы UI
    QMenuBar *menuBar;
    QToolBar *toolBar;
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    SecureTable *passwordTable;
    QStatusBar *statusBar;

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

private slots:
    void onInactivityTimeout();
    void onLock();

    // Обработчики меню
    void onNewDatabase();
    void onOpenDatabase();
    void onBackup();
    void onExit();
    void onAddEntry();
    void onEditEntry();
    void onDeleteEntry();
    void onViewLogs();
    void onSettings();
    void onAbout();
    void onFirstRunWizard();
    void onChangePassword();

public:
    MainWindow(ConfigHander &cfg, Database &database);
    ~MainWindow();

    void loadSampleData();
    void updateStatusBar();
};

#endif // MAINWINDOW_H
