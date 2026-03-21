#include "src/core/config_handler.h"
#include "src/database/DB_helper/db_helper.h"
#include "src/gui/MainWindow.h"
#include <wx/wx.h>

// Простое приложение wxWidgets
class CryptoSafeApp : public wxApp
{
public:
  virtual bool OnInit() override
  {
    // Создаем конфиг
    static ConfigHander config;

    // Создаем базу данных
    std::string dbPath = config.getDatabasePath();
    std::cout << "bd path: " << dbPath << std::endl;

    // Создаем базу данных с путем из конфига
    static Database db(dbPath);
    db.initialize();

    // Создаем главное окно
    MainWindow *frame = new MainWindow(config, db);
    frame->Show(true);

    return true;
  }
};

wxIMPLEMENT_APP(CryptoSafeApp);
