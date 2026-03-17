#ifndef SECURETABLE_H
#define SECURETABLE_H

#include "../src/database/DB_helper/db_helper.h"
#include <wx/listctrl.h>
#include <wx/wx.h>

class SecureTable : public wxListCtrl
{
private:
  void initColumns();

public:
  SecureTable(wxWindow *parent, wxWindowID id = wxID_ANY,
              const wxPoint &pos = wxDefaultPosition,
              const wxSize &size = wxDefaultSize);

  void addEntry(const VaultEntry &entry);
  void addSampleData();
  void clearAll();
  long getSelectedId();

  // Переопределяем для поддержки сортировки
  virtual wxString OnGetItemText(long item, long column) const;
};

#endif