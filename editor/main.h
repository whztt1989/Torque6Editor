//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _MAIN_H_
#define _MAIN_H_

// For compilers that don't support precompilation, include "wx/wx.h"
#include "wx/wxprec.h"
 
#ifndef WX_PRECOMP
#	include "wx/wx.h"
#endif

#ifndef PROJECT_WINDOW_H
#include "windows/project/projectWindow.h"
#endif

#ifndef CONSOLE_WINDOW_H
#include "windows/console/consoleWindow.h"
#endif

#ifndef MATERIALS_WINDOW_H
#include "windows/materials/materialsWindow.h"
#endif

#ifndef PROFILER_WINDOW_H
#include "windows/profiler/profilerWindow.h"
#endif

#ifndef SCENE_WINDOW_H
#include "windows/scene/sceneWindow.h"
#endif

#ifndef _SCRIPTS_TOOL_H_
#include "windows/scripts/scriptsWindow.h"
#endif

#ifndef EDITORMANAGER_H
#include "editorManager.h"
#endif

#ifndef __TORQUE6EDITORUI_H__
#include "Torque6EditorUI.h"
#endif

class Torque6Editor : public wxApp
{
public:
   ~Torque6Editor();

   // Window Management
   MainFrame*        mFrame;
   wxAuiManager*     mManager;

   // Editor Manager
   EditorManager     mEditorManager;

   // Dialogs
   AboutDialog*      mAboutDialog;

   // Events
	virtual bool OnInit();
   virtual void OnMenuEvent( wxCommandEvent& evt );
};
 
DECLARE_APP(Torque6Editor)
 
#endif // _MAIN_H_