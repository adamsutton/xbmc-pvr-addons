/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "CGUIDialogDeleteTimer.h"
#include "libXBMC_gui.h"

#define BUTTON_DELETE_ONE                 21
#define BUTTON_DELETE_SERIES              22
#define BUTTON_CLOSE                      30
#define LABEL_R1                          31
#define LABEL_R2                          32
#define LABEL_R3                          33


CGUIDialogDeleteTimer::CGUIDialogDeleteTimer(std::string r1, std::string r2, std::string r3)
{
  _r1 = r1;
  _r2 = r2;
  _r3 = r3;

  // specify the xml of the window here
  _returnValue = -1;
  _window = GUI->Window_create("DialogDeleteTimer.xml", "Confluence", false, true);

  // needed for every dialog
  _window->m_cbhdl = this;
  _window->CBOnInit = OnInitCB;
  _window->CBOnFocus = OnFocusCB;
  _window->CBOnClick = OnClickCB;
  _window->CBOnAction = OnActionCB;
}

CGUIDialogDeleteTimer::~CGUIDialogDeleteTimer()
{
  GUI->Window_destroy(_window);
}

bool CGUIDialogDeleteTimer::OnInit()
{
  _window->SetControlLabel(LABEL_R1, _r1.c_str());
  _window->SetControlLabel(LABEL_R2, _r2.c_str());
  _window->SetControlLabel(LABEL_R3, _r3.c_str());
  return true;
}

bool CGUIDialogDeleteTimer::OnClick(int controlId)
{
  switch(controlId)
  {
  case BUTTON_DELETE_ONE:
    _returnValue = 1;
    _window->Close();
    break;
  case BUTTON_DELETE_SERIES:
    _returnValue = 2;
    _window->Close();
    break;
  case BUTTON_CLOSE:
    _returnValue = 0;
    _window->Close();
    break;
  }
  return true;
}

bool CGUIDialogDeleteTimer::OnInitCB(GUIHANDLE cbhdl)
{
  CGUIDialogDeleteTimer* dialog = static_cast<CGUIDialogDeleteTimer*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogDeleteTimer::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogDeleteTimer* dialog = static_cast<CGUIDialogDeleteTimer*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogDeleteTimer::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogDeleteTimer* dialog = static_cast<CGUIDialogDeleteTimer*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogDeleteTimer::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CGUIDialogDeleteTimer* dialog = static_cast<CGUIDialogDeleteTimer*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CGUIDialogDeleteTimer::Show()
{
  if (_window)
    return _window->Show();

  return false;
}

void CGUIDialogDeleteTimer::Close()
{
  if (_window)
    _window->Close();
}

int CGUIDialogDeleteTimer::DoModal()
{
  if (_window)
    _window->DoModal();
  return _returnValue;		// return true if user didn't cancel dialog
}

bool CGUIDialogDeleteTimer::OnFocus(int controlId)
{
  return true;
}

bool CGUIDialogDeleteTimer::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU || actionId == 92/*back*/)
    return OnClick(BUTTON_CLOSE);
  else
    return false;
}
