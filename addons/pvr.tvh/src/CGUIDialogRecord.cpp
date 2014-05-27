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

#include "CGUIDialogRecord.h"
#include "libXBMC_gui.h"

#define BUTTON_REC_ONCE                   21
#define BUTTON_REC_EVERYTIME              22
#define BUTTON_REC_EVERY_WEEK_THIS_TIME   23
#define BUTTON_REC_EVERY_DAY_THIS_TIME    24
#define BUTTON_REC_WEEKDAYS               25
#define BUTTON_REC_WEEKENDS               26
#define BUTTON_CLOSE                      30
#define LABEL_SHOWNAME                    31

CGUIDialogRecord::CGUIDialogRecord(std::string showName) 
{
  _showName = showName;
  _returnValue = -1;

  _window = GUI->Window_create("DialogRecord.xml", "Confluence", false, true);
  _window->m_cbhdl = this;
  _window->CBOnInit = OnInitCB;
  _window->CBOnFocus = OnFocusCB;
  _window->CBOnClick = OnClickCB;
  _window->CBOnAction = OnActionCB;
}

CGUIDialogRecord::~CGUIDialogRecord()
{
  GUI->Window_destroy(_window);
}

bool CGUIDialogRecord::OnInit()
{
  // display the show name in the window
  _window->SetControlLabel(LABEL_SHOWNAME, _showName.c_str());
  return true;
}

bool CGUIDialogRecord::OnClick(int controlId)
{
  switch(controlId)
  {
  case BUTTON_REC_ONCE:
    _returnValue = REC_ONCE;
    _window->Close();
    break;
  case BUTTON_REC_EVERYTIME:
    _returnValue = REC_EVERYTIME;
    _window->Close();
    break;
  case BUTTON_REC_EVERY_WEEK_THIS_TIME:
    _returnValue = REC_EVERY_WEEK_THIS_TIME;
    _window->Close();
    break;
  case BUTTON_REC_EVERY_DAY_THIS_TIME:
    _returnValue = REC_EVERY_DAY_THIS_TIME;
    _window->Close();
    break;
  case BUTTON_REC_WEEKENDS:
    _returnValue = REC_WEEKENDS;
    _window->Close();
    break;
  case BUTTON_REC_WEEKDAYS:
    _returnValue = REC_WEEKDAYS;
    _window->Close();
    break;
  case BUTTON_CLOSE:
    _returnValue = 0;
    _window->Close();
    break;
  }

  return true;
}

bool CGUIDialogRecord::OnInitCB(GUIHANDLE cbhdl)
{
  CGUIDialogRecord* dialog = static_cast<CGUIDialogRecord*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogRecord::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogRecord* dialog = static_cast<CGUIDialogRecord*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogRecord::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  CGUIDialogRecord* dialog = static_cast<CGUIDialogRecord*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogRecord::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  CGUIDialogRecord* dialog = static_cast<CGUIDialogRecord*>(cbhdl);
  return dialog->OnAction(actionId);
}

bool CGUIDialogRecord::Show()
{
  if (_window)
    return _window->Show();

  return false;
}

void CGUIDialogRecord::Close()
{
  if (_window)
    _window->Close();
}

int CGUIDialogRecord::DoModal()
{
  if (_window)
    _window->DoModal();
  return _returnValue;		
}

bool CGUIDialogRecord::OnFocus(int controlId)
{
  return true;
}

bool CGUIDialogRecord::OnAction(int actionId)
{
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU || actionId == 92/*back*/)
    return OnClick(BUTTON_CLOSE);
  else
    return false;
}
