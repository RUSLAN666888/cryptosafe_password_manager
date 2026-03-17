#ifndef PASSWORDENTRY_H
#define PASSWORDENTRY_H

#include <wx/textctrl.h>
#include <wx/wx.h>

class PasswordEntry : public wxPanel
{
private:
  wxTextCtrl *passwordInput;
  wxCheckBox *showPasswordCheck;

  bool passwordVisible;

  void onShowPassword(wxCommandEvent &event);

public:
  PasswordEntry(wxWindow *parent, wxWindowID id = wxID_ANY,
                const wxString &value = "",
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize);

  ~PasswordEntry(); // Деструктор для затирания

  wxString GetValue() const;
  void SetValue(const wxString &value);
  void SetEditable(bool editable);

  wxDECLARE_EVENT_TABLE();
};

#endif