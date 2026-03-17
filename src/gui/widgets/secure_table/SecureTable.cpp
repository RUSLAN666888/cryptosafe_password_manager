#include "SecureTable.h"
#include <vector>

struct TableEntry
{
  long id;
  wxString title;
  wxString username;
  wxString url;
  wxString tags;
  wxString created;
};

static std::vector<TableEntry> g_entries;

SecureTable::SecureTable(wxWindow *parent, wxWindowID id, const wxPoint &pos,
                         const wxSize &size)
    : wxListCtrl(parent, id, pos, size,
                 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES)
{

  initColumns();
}

void SecureTable::initColumns()
{
  AppendColumn("ID", wxLIST_FORMAT_LEFT, 50);
  AppendColumn("Title", wxLIST_FORMAT_LEFT, 200);
  AppendColumn("Username", wxLIST_FORMAT_LEFT, 150);
  AppendColumn("URL", wxLIST_FORMAT_LEFT, 200);
  AppendColumn("Tags", wxLIST_FORMAT_LEFT, 150);
  AppendColumn("Created", wxLIST_FORMAT_LEFT, 150);
}

void SecureTable::addEntry(const VaultEntry &entry)
{
  TableEntry te;
  te.id = entry.id;
  te.title = entry.title;
  te.username = entry.username;
  te.url = entry.url;
  te.tags = entry.tags;
  te.created = entry.created_at;

  g_entries.push_back(te);

  long index = GetItemCount();
  InsertItem(index, wxString::Format("%ld", entry.id));
  SetItem(index, 1, entry.title);
  SetItem(index, 2, entry.username);
  SetItem(index, 3, entry.url);
  SetItem(index, 4, entry.tags);
  SetItem(index, 5, entry.created_at);
}

void SecureTable::addSampleData()
{
  // Тестовые данные для демонстрации
  VaultEntry e1;
  e1.id = 1;
  e1.title = "GitHub";
  e1.username = "user123";
  e1.url = "https://github.com";
  e1.tags = "[\"work\", \"dev\"]";
  e1.created_at = "2024-01-15";
  addEntry(e1);

  VaultEntry e2;
  e2.id = 2;
  e2.title = "Gmail";
  e2.username = "alice@gmail.com";
  e2.url = "https://gmail.com";
  e2.tags = "[\"personal\"]";
  e2.created_at = "2024-01-16";
  addEntry(e2);

  VaultEntry e3;
  e3.id = 3;
  e3.title = "Facebook";
  e3.username = "alice123";
  e3.url = "https://facebook.com";
  e3.tags = "[\"social\"]";
  e3.created_at = "2024-01-17";
  addEntry(e3);
}

void SecureTable::clearAll()
{
  DeleteAllItems();
  g_entries.clear();
}

long SecureTable::getSelectedId()
{
  long item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
  if (item == -1)
    return -1;

  wxString idStr = GetItemText(item, 0);
  long id;
  idStr.ToLong(&id);
  return id;
}

wxString SecureTable::OnGetItemText(long item, long column) const
{
  if (item < 0 || item >= (long)g_entries.size())
    return "";

  const TableEntry &entry = g_entries[item];

  switch (column)
  {
  case 0:
    return wxString::Format("%ld", entry.id);
  case 1:
    return entry.title;
  case 2:
    return entry.username;
  case 3:
    return entry.url;
  case 4:
    return entry.tags;
  case 5:
    return entry.created;
  default:
    return "";
  }
}