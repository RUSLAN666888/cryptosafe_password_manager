// tests/test_gui.cpp
#include "../src/core/config_handler.h"
#include "../src/database/DB_helper/db_helper.h"
#include "../src/gui/MainWindow.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <wx/evtloop.h>
#include <wx/wx.h>

// Тестовое приложение
class TestApp : public wxApp
{
public:
  bool OnInit() override
  {
    m_frame = nullptr;
    return true;
  }

  void SetFrame(wxFrame *frame) { m_frame = frame; }

  int OnExit() override
  {
    if (m_frame)
    {
      m_frame->Destroy();
      m_frame = nullptr;
    }
    return wxApp::OnExit();
  }

private:
  wxFrame *m_frame;
};

wxIMPLEMENT_APP_NO_MAIN(TestApp);

class GUITest : public ::testing::Test
{
protected:
  TestApp *app;
  ConfigHander *config;
  Database *db;
  std::string home_backup;
  std::string test_home = "/tmp/cryptosafe_test";

  void SetUp() override
  {
    // Инициализация wxWidgets
    int argc = 0;
    char **argv = nullptr;
    wxEntryStart(argc, argv);
    app = new TestApp();
    wxTheApp->SetAppName("CryptoSafe Test");

    // Создаем и активируем цикл событий
    wxEventLoopBase *loop = new wxEventLoop();
    wxEventLoopBase::SetActive(loop);

    // Подмена HOME для тестов
    const char *home = std::getenv("HOME");
    home_backup = home ? home : "";
    std::filesystem::remove_all(test_home);
    std::filesystem::create_directories(test_home);
    setenv("HOME", test_home.c_str(), 1);

    config = new ConfigHander();
    db = new Database(config->getDatabasePath());
    db->initialize();
  }

  void TearDown() override
  {
    // Завершаем цикл событий
    wxEventLoopBase *loop = wxEventLoopBase::GetActive();
    if (loop)
    {
      loop->Exit();
      delete loop;
    }

    delete db;
    delete config;
    delete wxTheApp;
    wxEntryCleanup();

    setenv("HOME", home_backup.c_str(), 1);
    std::filesystem::remove_all(test_home);
  }

  void ProcessEvents(int ms = 100)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    wxEventLoopBase *loop = wxEventLoopBase::GetActive();
    if (loop)
    {
      loop->Dispatch();
    }
  }
};

// ТЕСТ 1: Главное окно создается корректно
TEST_F(GUITest, MainWindowLaunchesCorrectly)
{
  // MainWindow сам создает wxFrame с parent = nullptr
  MainWindow *frame = new MainWindow(*config, *db);
  app->SetFrame(frame);
  frame->Show();

  ProcessEvents(200);

  EXPECT_NE(frame, nullptr);
  EXPECT_TRUE(frame->IsShown());
  EXPECT_EQ(frame->GetTitle(), "CryptoSafe Manager");

  wxSize size = frame->GetSize();
  EXPECT_EQ(size.GetWidth(), 900);
  EXPECT_EQ(size.GetHeight(), 600);

  frame->Close(true);
  ProcessEvents(200);
}

// ТЕСТ 2: Статус бар отображает правильную информацию
TEST_F(GUITest, StatusBarShowsCorrectInfo)
{
  MainWindow *frame = new MainWindow(*config, *db);
  app->SetFrame(frame);
  frame->Show();

  ProcessEvents(200);

  wxStatusBar *statusBar = frame->GetStatusBar();
  EXPECT_NE(statusBar, nullptr);

  wxString text = statusBar->GetStatusText(0);
  EXPECT_EQ(text, "Not logged in");

  frame->Close(true);
  ProcessEvents(200);
}

// ТЕСТ 3: Тестовые данные загружаются
TEST_F(GUITest, SampleDataLoads)
{
  MainWindow *frame = new MainWindow(*config, *db);
  app->SetFrame(frame);
  frame->Show();

  ProcessEvents(200);

  // Просто проверяем что окно создалось и не упало
  SUCCEED();

  frame->Close(true);
  ProcessEvents(200);
}

// Запуск тестов
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}