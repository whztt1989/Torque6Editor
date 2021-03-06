//-----------------------------------------------------------------------------
// Copyright (c) 2016 Andrew Mac
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

// For compilers that don't support precompilation, include "wx/wx.h"
#include "wx/wxprec.h"
 
#ifndef WX_PRECOMP
#	include "wx/wx.h"
#endif

#include <wx/dir.h>
#include <wx/utils.h> 
#include "wx/treectrl.h"
#include "wx/aui/aui.h"
#include "wx/dnd.h"

#include "platform/event.h"

// UI generated from wxFormBuilder
#include "Torque6EditorUI.h"

#include "widgets/wxTorqueAssetBrowser/wxTorqueAssetSelectDialog.h"
#include "widgets/wxTorqueAssetBrowser/wxTorqueAssetTree.h"
#include "theme.h"
#include "windows/materials/materialsWindow.h"

#include "editorManager.h"
#include "module/moduleManager.h"
#include "scene/components/meshComponent.h"
#include "rendering/renderCamera.h"

// Test Event
wxDEFINE_EVENT(wxTORQUE_SELECT_OBJECT, wxTorqueObjectEvent);

class TextDropTarget : public wxTextDropTarget
{
   virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text);
};

EditorManager::EditorManager()
   :  Debug::DebugMode(),
      Graphics::DGLHook(),
      mManager(NULL),
      mFrame(NULL),
      mWindow(NULL),
      mProjectLoaded(false), 
      mScenePlaying(false),
      mProjectName(""),
      mProjectPath(""),
      mTorque6Library(NULL),
      mTorque6Init(NULL),
      mTorque6Shutdown(NULL),
      mEditorOverlayView(NULL)
{
   mEditorCameraForwardVelocity  = Point3F::Zero;
   mEditorCameraSpeed            = 0.25f;

   mCommonIcons   = new wxImageList(16, 16);
   mTimer         = new wxTimer(this);
}

EditorManager::~EditorManager()
{
   closeProject();
   if (mTorque6Library != NULL)
      FreeLibrary(mTorque6Library);
}

void EditorManager::init(wxString runPath, wxAuiManager* manager, MainFrame* frame, wxWindow* window)
{
   mRunPath = runPath;
   mManager = manager;
   mFrame   = frame;
   mWindow  = window;

   // Common Icons
   mCommonIcons->Add(wxBitmap("images/moduleIcon.png", wxBITMAP_TYPE_PNG));
   mCommonIcons->Add(wxBitmap("images/iconFolderGrey.png", wxBITMAP_TYPE_PNG));
   mCommonIcons->Add(wxBitmap("images/assetIcon.png", wxBITMAP_TYPE_PNG));

   TextDropTarget* dropTarget = new TextDropTarget();
   mWindow->SetDropTarget(dropTarget);

   // Events
   mWindow->Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(EditorManager::OnIdle), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_SIZE, wxSizeEventHandler(EditorManager::OnSize), NULL, this);

   // Mouse Events
   mWindow->Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(EditorManager::OnMouseMove), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(EditorManager::OnMouseLeftDown), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(EditorManager::OnMouseLeftUp), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_RIGHT_DOWN, wxMouseEventHandler(EditorManager::OnMouseRightDown), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_RIGHT_UP, wxMouseEventHandler(EditorManager::OnMouseRightUp), NULL, this);

   // Keyboard Events
   mWindow->Connect(wxID_ANY, wxEVT_KEY_DOWN, wxKeyEventHandler(EditorManager::OnKeyDown), NULL, this);
   mWindow->Connect(wxID_ANY, wxEVT_KEY_UP, wxKeyEventHandler(EditorManager::OnKeyUp), NULL, this);

   // Add Tools to toolabr
   //mFrame->mainToolbar->AddTool(0, wxT("Run"), wxBitmap(wxT("images/run.png"), wxBITMAP_TYPE_ANY), wxNullBitmap, wxITEM_NORMAL, wxT("Run"), wxEmptyString, NULL);
   //mFrame->mainToolbar->AddSeparator();
   //mFrame->mainToolbar->Realize();

   // Toolbar Events
   //mFrame->mainToolbar->Connect(wxID_ANY, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(EditorManager::OnToolbarEvent), NULL, this);

   // Dialogs
   mAssetSelectDialog = new wxTorqueAssetSelectDialog(this, mWindow);

   // Dialogs - Material Selection Dialog
   mMaterialSelectDialog = new wxTorqueAssetSelectDialog(this, mWindow);

   // New Material Button
   mMaterialSelectDialog->TopBarSizer->Add(0, 0, 1, wxEXPAND, 5);
   wxButton* newMatButton = new wxButton(mMaterialSelectDialog, wxID_ANY, wxT("New Material"), wxDefaultPosition, wxDefaultSize, 0 | wxNO_BORDER);
   newMatButton->SetForegroundColour(wxColour(255, 255, 255));
   newMatButton->SetBackgroundColour(Theme::darkBackgroundColor);
   newMatButton->Bind(wxEVT_BUTTON, &EditorManager::OnNewMaterialButton, this, -1, -1, NULL);
   mMaterialSelectDialog->TopBarSizer->Add(newMatButton, 0, wxALL, 5);

   // Open in Material Editor Button
   wxButton* openMatButton = new wxButton(mMaterialSelectDialog, wxID_ANY, wxT("Open in Material Editor"), wxDefaultPosition, wxDefaultSize, 0 | wxNO_BORDER);
   openMatButton->SetForegroundColour(wxColour(255, 255, 255));
   openMatButton->SetBackgroundColour(Theme::darkBackgroundColor);
   openMatButton->Bind(wxEVT_BUTTON, &EditorManager::OnOpenInMaterialEditorButton, this, -1, -1, NULL);
   mMaterialSelectDialog->TopBarSizer->Add(openMatButton, 0, wxALL, 5);
   mMaterialSelectDialog->TopBarSizer->Add(0, 0, 1, wxEXPAND, 5);
}

bool EditorManager::openProject(wxString projectPath)
{
   // Load Torque 6 DLL
   if ( mTorque6Library == NULL )
   {
#ifdef TORQUE_DEBUG
      mTorque6Library = openLibrary("Torque6_DEBUG");
#else
      mTorque6Library = openLibrary("Torque6");
#endif

      // Load Necessary Functions
      mTorque6Init      = (initFunc)getLibraryFunc(mTorque6Library, "winInit");
      mTorque6Shutdown  = (shutdownFunc)getLibraryFunc(mTorque6Library, "winDestroy");
   }

   // If successful, initialize.
   if ( mTorque6Library && mTorque6Init && mTorque6Shutdown )
   {
      const char* argv[3];
      argv[0] = "Torque6Editor.exe";
      argv[1] = "-project";
      argv[2] = projectPath;

      mTorque6Init(3, argv, (HWND)mWindow->GetHWND());

      mProjectLoaded = true;
      mProjectPath = projectPath;
      wxDir projectDir(mProjectPath);
      mProjectName = projectDir.GetName();

      // Run a frame.
      Torque::Engine.mainLoop();

      // Editor Overlay
      Torque::Scene.pause();
      Torque::Debug.registerDebugMode("Editor", this);
      Torque::Debug.setDebugMode("Editor", true);

      // Editor Camera
      mEditorCamera.initialize(this);
      mEditorOverlayView = Torque::Graphics.getView("EditorOverlay", S32_MAX - 500, NULL);

      onProjectLoaded(mProjectName, projectPath);
      return true;
   }

   return false;
}

void EditorManager::closeProject()
{
   if (!mProjectLoaded) 
      return;

   mProjectLoaded = false;
   mProjectPath = "";
   mProjectName = "";

   mTorque6Shutdown();
   //FreeLibrary(T6_lib);

   onProjectClosed();
}

void EditorManager::runProject()
{
   wxString command = "";
   command.Append(mRunPath);
   command.Append(" -project ");
   command.Append(mProjectPath);
   wxExecute(command);
}

void EditorManager::play()
{
   mScenePlaying = true;
   Torque::Scene.save("editortemp.taml");
   Torque::Scene.play();
   mWindow->SetFocus();
}

void EditorManager::stop()
{
   mScenePlaying = false;
   Torque::Scene.stop();
   Torque::Scene.clear();
   Torque::Engine.mainLoop();

   Torque::Scene.load("editortemp.taml", false);
   Torque::Scene.play();
   Torque::Engine.mainLoop();

   wxRemoveFile("editortemp.taml");

   Torque::Scene.pause();
   mEditorCamera.initialize(this);
}

void EditorManager::OnToolbarEvent(wxCommandEvent& evt)
{
   switch (evt.GetId())
   {
      case 0:
         runProject();
         break;

      default:
         break;
   }
}

void EditorManager::OnIdle(wxIdleEvent& evt)
{
   if (mProjectLoaded)
   {
      if (!mScenePlaying)
         mEditorCamera.mainLoop();

      Torque::Engine.mainLoop();
      evt.RequestMore();
   }
}

void EditorManager::OnSize(wxSizeEvent& evt)
{
   if (!mProjectLoaded)
      return;

   Torque::Engine.resizeWindow(evt.GetSize().GetX(), evt.GetSize().GetY());
}

void EditorManager::OnMouseMove(wxMouseEvent& evt)
{
   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      mEditorCamera.onMouseMove(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onMouseMove(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onMouseMove(evt.GetPosition().x, evt.GetPosition().y);

   }
   else 
   {
      Torque::Engine.mouseMove(evt.GetPosition().x, evt.GetPosition().y);
   }
}

void EditorManager::OnMouseLeftDown(wxMouseEvent& evt)
{
   mWindow->SetFocus();

   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      mEditorCamera.onMouseLeftDown(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onMouseLeftDown(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onMouseLeftDown(evt.GetPosition().x, evt.GetPosition().y);
   }
   else {
      Torque::Engine.mouseButton(true, true);
   }
}

void EditorManager::OnMouseLeftUp(wxMouseEvent& evt)
{
   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      mEditorCamera.onMouseLeftUp(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onMouseLeftUp(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onMouseLeftUp(evt.GetPosition().x, evt.GetPosition().y);
   }
   else
   {
      Torque::Engine.mouseButton(false, true);
   }  
}

void EditorManager::OnMouseRightDown(wxMouseEvent& evt)
{
   mWindow->SetFocus();

   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      mEditorCamera.onMouseRightDown(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onMouseRightDown(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onMouseRightDown(evt.GetPosition().x, evt.GetPosition().y);
   }
   else
   {
      Torque::Engine.mouseButton(true, false);
   }
}

void EditorManager::OnMouseRightUp(wxMouseEvent& evt)
{
   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      mEditorCamera.onMouseRightUp(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onMouseRightUp(evt.GetPosition().x, evt.GetPosition().y);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onMouseRightUp(evt.GetPosition().x, evt.GetPosition().y);
   }
   else
   {
      Torque::Engine.mouseButton(false, false);
   }
}

KeyCodes getTorqueKeyCode(int key)
{
   switch(key)
   {
      case 65 : return KEY_A;
      case 66 : return KEY_B;
      case 67 : return KEY_C;
      case 68 : return KEY_D;
      case 69 : return KEY_E;
      case 70 : return KEY_F;
      case 71 : return KEY_G;
      case 72 : return KEY_H;
      case 73 : return KEY_I;
      case 74 : return KEY_J;
      case 75 : return KEY_K;
      case 76 : return KEY_L;
      case 77 : return KEY_M;
      case 78 : return KEY_N;
      case 79 : return KEY_O;
      case 80 : return KEY_P;
      case 81 : return KEY_Q;
      case 82 : return KEY_R;
      case 83 : return KEY_S;
      case 84 : return KEY_T;
      case 85 : return KEY_U;
      case 86 : return KEY_V;
      case 87 : return KEY_W;
      case 88 : return KEY_X;
      case 89 : return KEY_Y;
      case 90 : return KEY_Z;
   }

   return KEY_NULL;
}

void EditorManager::OnKeyDown(wxKeyEvent& evt)
{
   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      switch (evt.GetKeyCode())
      {
         case 87: // W
            mEditorCameraForwardVelocity.x = 1.0 * mEditorCameraSpeed;
            break;

         case 65: // A
            mEditorCameraForwardVelocity.y = 1.0 * mEditorCameraSpeed;
            break;

         case 83: // S
            mEditorCameraForwardVelocity.x = -1.0 * mEditorCameraSpeed;
            break;

         case 68: // D
            mEditorCameraForwardVelocity.y = -1.0 * mEditorCameraSpeed;
            break;
      }

      mEditorCamera.setShiftKey(evt.ShiftDown());
      mEditorCamera.setForwardVelocity(mEditorCameraForwardVelocity);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onKeyDown(evt);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onKeyDown(evt);
   }
   else
   {
      KeyCodes torqueKey = getTorqueKeyCode(evt.GetKeyCode());
      Torque::Engine.keyDown(torqueKey);
   }
}

void EditorManager::OnKeyUp(wxKeyEvent& evt)
{
   if (!mProjectLoaded)
      return;

   if (!mScenePlaying)
   {
      switch (evt.GetKeyCode())
      {
         case 87: // W
            mEditorCameraForwardVelocity.x = 0.0;
            break;

         case 65: // A
            mEditorCameraForwardVelocity.y = 0.0;
            break;

         case 83: // S
            mEditorCameraForwardVelocity.x = 0.0;
            break;

         case 68: // D
            mEditorCameraForwardVelocity.y = 0.0;
            break;
      }

      mEditorCamera.setShiftKey(evt.ShiftDown());
      mEditorCamera.setForwardVelocity(mEditorCameraForwardVelocity);

      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->onKeyUp(evt);

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onKeyUp(evt);
   }
   else
   {
      switch (evt.GetKeyCode())
      {
         case WXK_ESCAPE: // Escape
            stop();
            return;
      }

      KeyCodes torqueKey = getTorqueKeyCode(evt.GetKeyCode());
      Torque::Engine.keyUp(torqueKey);
   }
}

void EditorManager::render(Rendering::RenderCamera* camera)
{
   //Torque::bgfx.setViewRect(mEditorOverlayView->id, 0, 0, *Torque::Rendering.windowWidth, *Torque::Rendering.windowHeight);
   //Torque::bgfx.setViewTransform(mEditorOverlayView->id, camera->viewMatrix, camera->projectionMatrix, BGFX_VIEW_STEREO, NULL);

   if (!mScenePlaying)
   {
      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->renderTool();

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->renderWindow();
   }
}

void EditorManager::dglRender()
{
   if (!mScenePlaying)
   {
      for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
         EditorTool::smEditorTools[i]->dglRenderTool();

      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->dglRenderWindow();
   }
}

void EditorManager::onProjectLoaded(const wxString& projectName, const wxString& projectPath)
{
   for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
      EditorTool::smEditorTools[i]->onProjectLoaded(projectName, projectPath);

   for(unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
      EditorWindow::smEditorWindows[i]->onProjectLoaded(projectName, projectPath);
}

void EditorManager::onProjectClosed()
{
   for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
      EditorTool::smEditorTools[i]->onProjectClosed();

   for(unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
      EditorWindow::smEditorWindows[i]->onProjectClosed();
}

// ---------------------------------------------------------------

void _addObjectTemplateAsset(wxString assetID, Point3F position)
{
   Scene::SceneObject* newObject = new Scene::SceneObject();
   newObject->setTemplateAsset(assetID);
   newObject->mTransform.setPosition(position);
   Torque::Scene.addObject(newObject, "NewSceneObject");
}

void _addMeshAsset(wxString assetID, Point3F position)
{
   Scene::SceneObject* newObject = new Scene::SceneObject();
   Scene::MeshComponent* meshComponent = new Scene::MeshComponent();
   meshComponent->setMesh(assetID.c_str());
   newObject->addComponent(meshComponent);
   newObject->mTransform.setPosition(position);
   Torque::Scene.addObject(newObject, "NewSceneObject");
   meshComponent->registerObject();
}

bool TextDropTarget::OnDropText(wxCoord x, wxCoord y, const wxString& text)
{
   // Debug:
   Torque::Con.printf("DROP: x: %d y: %d text: %s", x, y, static_cast<const char*>(text));

   // Parse Commands
   wxStringTokenizer tokenizer(text, "->");
   wxString command = tokenizer.GetNextToken();

   // Asset->AssetType->AssetID
   if (command == "Asset" && tokenizer.HasMoreTokens())
   {
      tokenizer.GetNextToken();
      wxString assetType = tokenizer.GetNextToken();
      tokenizer.GetNextToken();
      wxString assetID = tokenizer.GetNextToken();

      Point3F nearPoint;
      Point3F farPoint;
      Torque::Rendering.screenToWorld(Point2I(x, y), nearPoint, farPoint);

      Point3F direction = farPoint - nearPoint;
      direction.normalize();

      // ObjectTemplateAsset gets added straight to the scene.
      if (assetType == "ObjectTemplateAsset")
         _addObjectTemplateAsset(assetID, nearPoint + (direction * 10.0f));

      // MeshAsset
      if (assetType == "MeshAsset")
         _addMeshAsset(assetID, nearPoint + (direction * 10.0f));

      // Inform the tools the scene has changed.
      for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
         EditorWindow::smEditorWindows[i]->onSceneChanged();
   }

   return true;
}

void EditorManager::addObjectTemplateAsset(wxString assetID, Point3F position)
{
   _addObjectTemplateAsset(assetID, position);
}

void EditorManager::addMeshAsset(wxString assetID, Point3F position)
{
   _addMeshAsset(assetID, position);
}

void EditorManager::postEvent(const wxEvent& evt)
{
   for (unsigned int i = 0; i < EditorTool::smEditorTools.size(); ++i)
      wxPostEvent(EditorTool::smEditorTools[i], evt);

   for (unsigned int i = 0; i < EditorWindow::smEditorWindows.size(); ++i)
      wxPostEvent(EditorWindow::smEditorWindows[i], evt);

   // Note: this forces events to process asap.
   mTimer->StartOnce(0);
}

void EditorManager::refreshChoices()
{
   if (!isProjectLoaded())
      return;

   mTextureAssetChoices.Clear();
   mTextureAssetChoices.Add("", 0);

   Vector<const AssetDefinition*> assetDefinitions = Torque::AssetDatabaseLink.getDeclaredAssets();

   // Iterate sorted asset definitions.
   for (Vector<const AssetDefinition*>::iterator assetItr = assetDefinitions.begin(); assetItr != assetDefinitions.end(); ++assetItr)
   {
      // Fetch asset definition.
      const AssetDefinition* pAssetDefinition = *assetItr;

      // Populate TextureAsset choices menu.
      if (dStrcmp(pAssetDefinition->mAssetType, "TextureAsset") == 0)
         mTextureAssetChoices.Add(pAssetDefinition->mAssetId, mTextureAssetChoices.GetCount());
   }
}

void EditorManager::refreshModuleList()
{
   mModuleList.clear();
   Vector<const AssetDefinition*> assetDefinitions = Torque::AssetDatabaseLink.getDeclaredAssets();

   // Fetch all loaded module definitions.
   ModuleManager::typeConstModuleDefinitionVector loadedModules;
   Torque::ModuleDatabaseLink->findModules(true, loadedModules);

   // Iterate found loaded module definitions.
   for (ModuleManager::typeConstModuleDefinitionVector::const_iterator loadedModuleItr = loadedModules.begin(); loadedModuleItr != loadedModules.end(); ++loadedModuleItr)
   {
      // Fetch module definition.
      const ModuleDefinition* module = *loadedModuleItr;

      // Add to module list.
      ModuleInfo newModule;
      newModule.moduleID = module->getModuleId();
      newModule.moduleVersion = module->getVersionId();
      mModuleList.push_back(newModule);
   }

   // Iterate asset definitions.
   for (Vector<const AssetDefinition*>::iterator assetItr = assetDefinitions.begin(); assetItr != assetDefinitions.end(); ++assetItr)
   {
      // Fetch asset definition.
      const AssetDefinition* pAssetDefinition = *assetItr;

      char buf[256];
      dStrcpy(buf, pAssetDefinition->mAssetId);
      const char* moduleName = dStrtok(buf, ":");
      const char* assetName = dStrtok(NULL, ":");

      // Try to find module
      bool foundModule = false;
      for (Vector<ModuleInfo>::iterator modulesItr = mModuleList.begin(); modulesItr != mModuleList.end(); ++modulesItr)
      {
         const char* moduleID = pAssetDefinition->mpModuleDefinition->getModuleId();
         if (dStrcmp(modulesItr->moduleID, moduleID) == 0)
         {
            // Try to find category
            bool foundCategory = false;
            for (Vector<AssetCategoryInfo>::iterator categoriesItr = modulesItr->assets.begin(); categoriesItr != modulesItr->assets.end(); ++categoriesItr)
            {
               const char* moduleID = pAssetDefinition->mpModuleDefinition->getModuleId();
               if (dStrcmp(categoriesItr->categoryName, pAssetDefinition->mAssetType) == 0)
               {
                  categoriesItr->assets.push_back(pAssetDefinition);
                  foundCategory = true;
                  break;
               }
            }

            // Can't find module? Create one.
            if (!foundCategory)
            {
               AssetCategoryInfo newCategory;
               newCategory.categoryName = pAssetDefinition->mAssetType;
               newCategory.assets.push_back(pAssetDefinition);
               modulesItr->assets.push_back(newCategory);
            }

            foundModule = true;
            break;
         }
      }

      // Can't find module? Create one.
      if (!foundModule)
      {
         ModuleInfo newModule;
         newModule.moduleID = pAssetDefinition->mpModuleDefinition->getModuleId();
         newModule.moduleVersion = pAssetDefinition->mpModuleDefinition->getVersionId();

         AssetCategoryInfo newCategory;
         newCategory.categoryName = pAssetDefinition->mAssetType;
         newCategory.assets.push_back(pAssetDefinition);
         newModule.assets.push_back(newCategory);
         mModuleList.push_back(newModule);
      }
   }
}

Vector<ModuleInfo>* EditorManager::getModuleList()
{
   refreshModuleList();
   return &mModuleList;
}

wxPGChoices* EditorManager::getTextureAssetChoices()
{
   refreshChoices();
   return &mTextureAssetChoices;
}

bool EditorManager::selectMaterial(wxString& returnMaterialName, const char* defaultAsset)
{
   return mMaterialSelectDialog->SelectAsset(returnMaterialName, "MaterialAsset", defaultAsset);
}

bool EditorManager::selectAsset(wxString& returnValue, const char* filter, const char* defaultAsset)
{
   return mAssetSelectDialog->SelectAsset(returnValue, filter, defaultAsset);
}

void EditorManager::OnNewMaterialButton(wxCommandEvent& evt)
{
   wxString newMaterialName;
   if (newMaterialWizard(newMaterialName))
   {
      mMaterialSelectDialog->SetSelectedAsset(newMaterialName);
      mMaterialSelectDialog->EndModal(1);
   }
}

void EditorManager::OnOpenInMaterialEditorButton(wxCommandEvent& evt)
{
   if (mMaterialSelectDialog->SelectedAsset != NULL)
   {
      mMaterialSelectDialog->EndModal(1);
      MaterialsWindow* materialsWindow = EditorWindow::findWindow<MaterialsWindow>();
      materialsWindow->openWindow();
      materialsWindow->openMaterial(mMaterialSelectDialog->SelectedAsset->mAssetId);
   }
}

void _materialWizardSelectModule(NewMaterialWizard* wizard)
{
   wxString selection = wizard->moduleSelection->GetString(wizard->moduleSelection->GetSelection());
   ModuleDefinition* module = Torque::ModuleDatabaseLink->findLoadedModule(selection.c_str());

   wxString defaultSavePath = module->getModulePath();
   defaultSavePath.Append("/materials");
   wizard->savePath->SetPath(defaultSavePath);
}

bool EditorManager::newMaterialWizard(wxString& returnMaterialName, const char* moduleId)
{
   NewMaterialWizard* wizard = new NewMaterialWizard(mFrame);
   wxWizardCallback* wizardCallback = new wxWizardCallback();
   wizardCallback->wizard = wizard;
   wizard->moduleSelection->Bind(wxEVT_CHOICE, &EditorManager::OnNewMaterialSelectModule, this, -1, -1, wizardCallback);

   // Populate module list.
   int selection = 0;
   for (S32 n = 0; n < mModuleList.size(); ++n)
   {
      if (moduleId != NULL && dStrcmp(mModuleList[n].moduleID, moduleId) == 0)
         selection = n;

      wizard->moduleSelection->AppendString(wxString(mModuleList[n].moduleID));
   }
   wizard->moduleSelection->Select(selection);
   _materialWizardSelectModule(wizard);

   if (wizard->RunWizard(wizard->m_pages[0]))
   {
      wxString assetID = wizard->assetID->GetValue();
      wxString savePath = wizard->savePath->GetPath();

      wxString selection = wizard->moduleSelection->GetString(wizard->moduleSelection->GetSelection());
      ModuleDefinition* module = Torque::ModuleDatabaseLink->findLoadedModule(selection.c_str());

      wxString assetPath("");
      assetPath.Append(savePath);
      assetPath.Append("/");
      assetPath.Append(assetID);
      assetPath.Append(".asset.taml");

      wxString templateFileName("");
      templateFileName.Append(assetID);
      templateFileName.Append(".taml");

      wxString templatePath("");
      templatePath.Append(savePath);
      templatePath.Append("/");
      templatePath.Append(templateFileName);

      // Create material template and then asset.
      Torque::Scene.createMaterialTemplate(templatePath.c_str());
      Torque::Scene.createMaterialAsset(assetID.c_str(), templateFileName.c_str(), assetPath.c_str());
      Torque::AssetDatabaseLink.addDeclaredAsset(module, assetPath.c_str());

      returnMaterialName = wxString("");
      returnMaterialName.Append(module->getModuleId());
      returnMaterialName.Append(":");
      returnMaterialName.Append(assetID);

      // Open new material in editor?
      if (wizard->openInMaterialEditor->GetValue())
      {
         MaterialsWindow* materialsWindow = EditorWindow::findWindow<MaterialsWindow>();
         materialsWindow->openWindow();
         materialsWindow->openMaterial(returnMaterialName.c_str());
      }

      return true;
   }

   return false;
}

void EditorManager::OnNewMaterialSelectModule(wxCommandEvent& evt)
{
   wxWizardCallback* callback = dynamic_cast<wxWizardCallback*>(evt.GetEventUserData());
   NewMaterialWizard* wizard = dynamic_cast<NewMaterialWizard*>(callback->wizard);
   if (!wizard)
      return;

   _materialWizardSelectModule(wizard);
}