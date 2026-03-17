#include "PasswordEntry.h"
#include "../src/core/memory_zero.h"
#include <wx/clipbrd.h>
#include <wx/msgdlg.h>

#define wxID_PasswordCheckBox 1
#define wxID_PasswordField 2

wxBEGIN_EVENT_TABLE(PasswordEntry, wxPanel)
    EVT_CHECKBOX(wxID_PasswordCheckBox, PasswordEntry::onShowPassword)
        wxEND_EVENT_TABLE()

            PasswordEntry::PasswordEntry(wxWindow *parent, wxWindowID id,
                                         const wxString &value,
                                         const wxPoint &pos, const wxSize &size)
    : wxPanel(parent, id, pos, size), passwordVisible(false)
{

  wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

  // Поле ввода пароля
  passwordInput =
      new wxTextCtrl(this, wxID_PasswordField, value, wxDefaultPosition,
                     wxSize(200, -1), wxTE_PASSWORD);

  // Чекбокс "Показать пароль"
  showPasswordCheck = new wxCheckBox(this, wxID_PasswordCheckBox, "Show");

  sizer->Add(passwordInput, 1, wxALL | wxEXPAND, 2);
  sizer->Add(showPasswordCheck, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);

  SetSizer(sizer);
  Layout();
}

PasswordEntry::~PasswordEntry()
{
  // Здесь будет затирание памяти в будущих спринтах
}

wxString PasswordEntry::GetValue() const { return passwordInput->GetValue(); }

void PasswordEntry::SetValue(const wxString &value)
{
  passwordInput->SetValue(value);
}

void PasswordEntry::SetEditable(bool editable)
{
  passwordInput->SetEditable(editable);
}

void PasswordEntry::onShowPassword(wxCommandEvent &event)
{
  passwordVisible = showPasswordCheck->IsChecked();

  // Получаем текущее значение
  wxString value = passwordInput->GetValue();

  // Сохраняем размеры старого поля
  wxSize oldSize = passwordInput->GetSize();

  // Создаём новое текстовое поле с нужным стилем
  int style = wxTE_PROCESS_ENTER;
  if (!passwordVisible)
  {
    style |= wxTE_PASSWORD; // Добавляем флаг скрытия
  }

  // Удаляем старое поле
  passwordInput->Destroy();

  // Создаём новое с тем же размером и стилем
  passwordInput = new wxTextCtrl(this, wxID_PasswordField, value,
                                 wxDefaultPosition, oldSize, style);

  // Обновляем layout
  GetSizer()->Insert(0, passwordInput, 1, wxALL | wxEXPAND, 2);
  GetSizer()->Layout();
}
