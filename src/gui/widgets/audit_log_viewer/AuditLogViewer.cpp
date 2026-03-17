#include "AuditLogViewer.h"
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(AuditLogViewer, wxDialog)
    EVT_BUTTON(wxID_ANY, AuditLogViewer::onRefresh) wxEND_EVENT_TABLE()

        AuditLogViewer::AuditLogViewer(wxWindow *parent, Database &database)
    : wxDialog(parent, wxID_ANY, "Audit Logs", wxDefaultPosition,
               wxSize(700, 500)),
      db(database)
{

  // Основной вертикальный sizer
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

  // Создаем список с колонками
  logList =
      new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);

  // Добавляем колонки
  logList->AppendColumn("ID", wxLIST_FORMAT_LEFT, 50);
  logList->AppendColumn("Action", wxLIST_FORMAT_LEFT, 150);
  logList->AppendColumn("Timestamp", wxLIST_FORMAT_LEFT, 180);
  logList->AppendColumn("Entry ID", wxLIST_FORMAT_LEFT, 80);
  logList->AppendColumn("Details", wxLIST_FORMAT_LEFT, 200);

  mainSizer->Add(logList, 1, wxEXPAND | wxALL, 5);

  // Панель с кнопками
  wxPanel *buttonPanel = new wxPanel(this);
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

  refreshButton = new wxButton(buttonPanel, wxID_ANY, "Refresh");
  closeButton = new wxButton(buttonPanel, wxID_CANCEL, "Close");

  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(refreshButton, 0, wxALL, 5);
  buttonSizer->Add(closeButton, 0, wxALL, 5);

  buttonPanel->SetSizer(buttonSizer);
  mainSizer->Add(buttonPanel, 0, wxEXPAND | wxALL, 5);

  SetSizer(mainSizer);

  // Загружаем логи
  refreshLogs();

  // Центрируем окно
  Centre();
}

void AuditLogViewer::refreshLogs()
{
  // Очищаем список
  logList->DeleteAllItems();

  // Получаем логи из базы данных
  auto logs = db.getAuditLogs(100); // Последние 100 записей

  int index = 0;
  for (const auto &log : logs)
  {
    // Вставляем новую строку
    long itemIndex =
        logList->InsertItem(index, wxString::Format("%ld", log.id));

    // Заполняем колонки
    logList->SetItem(itemIndex, 1, log.action);
    logList->SetItem(itemIndex, 2, log.timestamp);

    if (log.entry_id >= 0)
    {
      logList->SetItem(itemIndex, 3, wxString::Format("%d", log.entry_id));
    }
    else
    {
      logList->SetItem(itemIndex, 3, "-");
    }

    logList->SetItem(itemIndex, 4, log.details);

    index++;
  }

  // Если нет логов, показываем сообщение
  if (logs.empty())
  {
    long itemIndex = logList->InsertItem(0, "-");
    logList->SetItem(itemIndex, 1, "No audit logs found");
    logList->SetItem(itemIndex, 2, "");
    logList->SetItem(itemIndex, 3, "");
    logList->SetItem(itemIndex, 4, "");
  }

  // Автоматически подгоняем ширину колонок
  for (int i = 0; i < 5; i++)
  {
    logList->SetColumnWidth(i, wxLIST_AUTOSIZE);
    // Минимальная ширина для некоторых колонок
    if (i == 4 && logList->GetColumnWidth(i) < 200)
    {
      logList->SetColumnWidth(i, 200);
    }
  }
}

void AuditLogViewer::onRefresh(wxCommandEvent &event) { refreshLogs(); }
