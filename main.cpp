#include <QApplication>
#include <QMessageBox>
#include <iostream>
#include "src/core/config_handler.h"
#include "src/database/DB_helper/db_helper.h"
#include "../src/core/vault/VaultManager.h"
#include "src/gui/MainWindow.h"
#include "../src/core/clipboard_service/clipboard_service.h"
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // QFile styleFile("/home/master666/development/cryptosafe_password_manager/dark_theme.qss");
    // if (styleFile.open(QFile::ReadOnly)) {
    //     app.setStyleSheet(QLatin1String(styleFile.readAll()));
    //     styleFile.close();
    // }

    // Устанавливаем имя приложения
    app.setApplicationName("CryptoSafe Manager");
    app.setOrganizationName("CryptoSafe");

    try
    {
        // Создаем конфиг
        static ConfigHander config;

        // Создаем базу данных
        std::string dbPath = config.getDatabasePath();
        std::cout << "Database path: " << dbPath << std::endl;

        // Создаем базу данных с путем из конфига
        static Database db(dbPath);

        if (!db.initialize())
        {
            QMessageBox::critical(nullptr, "Error",
                                  "Failed to initialize database!\n\n"
                                  "Please check permissions and try again.");
            return 1;
        }

        // Инициализация ClipboardService
        ClipboardService::getInstance().init(&db);
        ClipboardService::getInstance().loadNotificationSettings();
        ClipboardService::getInstance().checkAndRestoreTimer();
        //ClipboardService::getInstance().restoreRemainingTime();

        //AuditLogger::getInstance().init(db);
        //LogVerifier::getInstance().init(&db);

        static VaultManager m(db);

        // Создаем главное окно
        MainWindow mainWindow(config, db, m);
        mainWindow.show();

        return app.exec();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(nullptr, "Fatal Error",
                              QString("An unexpected error occurred:\n\n%1").arg(e.what()));
        return 1;
    }
    catch (...)
    {
        QMessageBox::critical(nullptr, "Fatal Error",
                              "An unknown error occurred!");
        return 1;
    }
}
