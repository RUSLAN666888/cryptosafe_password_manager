#ifndef AUDITLOGVIEWER_H
#define AUDITLOGVIEWER_H

#include "../src/database/DB_helper/db_helper.h"
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

class AuditLogViewer : public wxDialog
{
private:
  Database &db;
  wxListCtrl *logList;
  wxButton *refreshButton;
  wxButton *closeButton;

  void refreshLogs();
  void onRefresh(wxCommandEvent &event);

public:
  AuditLogViewer(wxWindow *parent, Database &database);

  wxDECLARE_EVENT_TABLE();
};

#endif // AUDITLOGVIEWER_