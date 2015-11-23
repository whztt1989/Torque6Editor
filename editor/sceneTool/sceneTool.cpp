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

// For compilers that don't support precompilation, include "wx/wx.h"
#include "wx/wxprec.h"
 
#ifndef WX_PRECOMP
#	include "wx/wx.h"
#endif

#include <wx/propgrid/propgrid.h>
#include <wx/dir.h>
#include <wx/treectrl.h>

// UI generated from wxFormBuilder
#include "../Torque6EditorUI.h"

#include "sceneTool.h"
#include "3d/scene/camera.h"
#include <bx/bx.h>
#include <bx/fpumath.h>

#include "3d/scene/object/components/animationComponent.h"
#include "3d/scene/object/components/controllerComponent.h"
#include "3d/scene/object/components/lightComponent.h"
#include "3d/scene/object/components/meshComponent.h"
#include "3d/scene/object/components/physicsComponent.h"
#include "3d/scene/object/components/textComponent.h"

SceneTool::SceneTool(ProjectManager* _projectManager, MainFrame* _frame, wxAuiManager* _manager)
   : Parent(_projectManager, _frame, _manager),
     mScenePanel(NULL),
     mSelectedObject(NULL),
     mSelectedComponent(NULL),
     mSelectedFeature(NULL),
     mMenuObject(NULL),
     mMenuComponent(NULL),
     mMenuFeature(NULL),
     mLightIcon(NULL),
     mRefreshing(false)
{
   mObjectIconList = new wxImageList(16, 16);
   mFeatureIconList = new wxImageList( 16, 16 );

   // Translate Menu
   mTranslateMenu = new wxMenu;
   mTranslateMenu->Append(0, wxT("Snap: None"), wxEmptyString, wxITEM_RADIO);
   mTranslateMenu->Append(1, wxT("Snap: 0.1"), wxEmptyString, wxITEM_RADIO);
   mTranslateMenu->Append(2, wxT("Snap: 0.5"), wxEmptyString, wxITEM_RADIO);
   mTranslateMenu->Append(3, wxT("Snap: 1.0"), wxEmptyString, wxITEM_RADIO);
   mTranslateMenu->Append(4, wxT("Snap: 5.0"), wxEmptyString, wxITEM_RADIO);
   mTranslateMenu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnTranslateMenuEvent), NULL, this);

   // Rotate Menu
   mRotateMenu = new wxMenu;
   mRotateMenu->Append(0, wxT("Snap: None"), wxEmptyString, wxITEM_RADIO);
   mRotateMenu->Append(1, wxT("Snap: 5 Degrees"), wxEmptyString, wxITEM_RADIO);
   mRotateMenu->Append(2, wxT("Snap: 15 Degrees"), wxEmptyString, wxITEM_RADIO);
   mRotateMenu->Append(3, wxT("Snap: 45 Degrees"), wxEmptyString, wxITEM_RADIO);
   mRotateMenu->Append(4, wxT("Snap: 90 Degrees"), wxEmptyString, wxITEM_RADIO);
   mRotateMenu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnRotateMenuEvent), NULL, this);

   // Scale Menu
   mScaleMenu = new wxMenu;
   mScaleMenu->Append(0, wxT("Snap: None"), wxEmptyString, wxITEM_RADIO);
   mScaleMenu->Append(1, wxT("Snap: 0.1"), wxEmptyString, wxITEM_RADIO);
   mScaleMenu->Append(2, wxT("Snap: 0.5"), wxEmptyString, wxITEM_RADIO);
   mScaleMenu->Append(3, wxT("Snap: 1.0"), wxEmptyString, wxITEM_RADIO);
   mScaleMenu->Append(4, wxT("Snap: 5.0"), wxEmptyString, wxITEM_RADIO);
   mScaleMenu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnScaleMenuEvent), NULL, this);
}

SceneTool::~SceneTool()
{
   mRefreshing = true;
   delete mTranslateMenu;
   delete mRotateMenu;
   delete mScaleMenu;
}

void SceneTool::initTool()
{
   mGizmo.mProjectManager = mProjectManager;
   mScenePanel = new ScenePanel(mFrame, wxID_ANY);

   mScenePanel->Connect(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SceneTool::OnMenuEvent), NULL, this);

   // Object Icons.
   mObjectIconList->Add(wxBitmap("images/objectIcon.png", wxBITMAP_TYPE_PNG));
   mObjectIconList->Add(wxBitmap("images/componentIcon.png", wxBITMAP_TYPE_PNG));
   mScenePanel->objectList->AssignImageList(mObjectIconList);

   // Object Events
   mScenePanel->objectList->Connect(wxID_ANY, wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(SceneTool::OnTreeEvent), NULL, this);
   mScenePanel->objectList->Connect(wxID_ANY, wxEVT_TREE_ITEM_MENU, wxTreeEventHandler(SceneTool::OnTreeMenu), NULL, this);
   mScenePanel->propertyGrid->Connect(wxID_ANY, wxEVT_PG_CHANGED, wxPropertyGridEventHandler(SceneTool::OnObjectPropChanged), NULL, this);

   // Feature Icons.
   mFeatureIconList->Add(wxBitmap("images/featureIcon.png", wxBITMAP_TYPE_PNG));
   mScenePanel->featureList->AssignImageList(mFeatureIconList);

   // Feature Events
   mScenePanel->featureList->Connect(wxID_ANY, wxEVT_TREE_SEL_CHANGED, wxTreeEventHandler(SceneTool::OnTreeEvent), NULL, this);
   mScenePanel->featureList->Connect(wxID_ANY, wxEVT_TREE_ITEM_MENU, wxTreeEventHandler(SceneTool::OnTreeMenu), NULL, this);
   mScenePanel->featurePropGrid->Connect(wxID_ANY, wxEVT_PG_CHANGED, wxPropertyGridEventHandler(SceneTool::OnFeaturePropChanged), NULL, this);

   mObjectListRoot = mScenePanel->objectList->AddRoot("ROOT");
   mFeatureListRoot = mScenePanel->featureList->AddRoot("ROOT");

   mManager->AddPane(mScenePanel, wxAuiPaneInfo().Caption("Scene")
                                                  .CaptionVisible( true )
                                                  .CloseButton( true )
                                                  .PinButton( true )
                                                  .MaximizeButton(true)
                                                  .Dock()
                                                  .Resizable()
                                                  .FloatingSize( wxDefaultSize )
                                                  .Left()
                                                  .Hide());
   mManager->Update();

   // Add Tools to toolabr
   mFrame->mainToolbar->AddTool(1, wxT("Move"), wxBitmap(wxT("images/translate.png"), wxBITMAP_TYPE_ANY), wxNullBitmap, wxITEM_DROPDOWN, wxT("Move"), wxEmptyString, NULL);
   mFrame->mainToolbar->AddTool(2, wxT("Rotate"), wxBitmap(wxT("images/rotate.png"), wxBITMAP_TYPE_ANY), wxNullBitmap, wxITEM_DROPDOWN, wxT("Rotate"), wxEmptyString, NULL);
   mFrame->mainToolbar->AddTool(3, wxT("Scale"), wxBitmap(wxT("images/scale.png"), wxBITMAP_TYPE_ANY), wxNullBitmap, wxITEM_DROPDOWN, wxT("Scale"), wxEmptyString, NULL);
   mFrame->mainToolbar->AddSeparator();
   mFrame->mainToolbar->Realize();

   // Toolbar Dropdown Events
   mFrame->mainToolbar->Connect(wxID_ANY, wxEVT_COMMAND_TOOL_DROPDOWN_CLICKED, wxCommandEventHandler(SceneTool::OnToolbarDropdownEvent), NULL, this);

   // Refresh Mesh and Material Choices
   refreshChoices();
}

void SceneTool::openTool()
{
   wxAuiPaneInfo& paneInfo = mManager->GetPane(mScenePanel);
   paneInfo.Show();
   mManager->Update();

   if (mProjectManager->mProjectLoaded)
   {
      refreshObjectList();
      refreshFeatureList();
      refreshChoices();
   }
}

void SceneTool::closeTool()
{
   wxAuiPaneInfo& paneInfo = mManager->GetPane(mScenePanel);
   paneInfo.Hide();
   mManager->Update();
}

void SceneTool::renderTool()
{
   // Draw Light Icons
   if (mLightIcon != NULL)
   {
      Vector<Rendering::LightData*> lightList = Plugins::Link.Rendering.getLightList();
      for (S32 n = 0; n < lightList.size(); ++n)
      {
         Rendering::LightData* light = lightList[n];

         Plugins::Link.Graphics.drawBillboard(mProjectManager->mRenderLayer4View->id,
                                              mLightIcon,
                                              light->position,
                                              1.0f, 1.0f,
                                              ColorI(light->color[0] * 255, light->color[1] * 255, light->color[2] * 255, 255),
                                              NULL);
      }
   }

   // Object Selected
   if (mSelectedObject != NULL && mSelectedComponent == NULL)
   {
      // Bounding Box
      Plugins::Link.Graphics.drawBox3D(mProjectManager->mRenderLayer4View->id, mSelectedObject->mBoundingBox, ColorI(255, 255, 255, 255), NULL);

      // Render Gizmo
      mGizmo.render();

      return;
   }

   // Component Selected
   if (mSelectedObject != NULL && mSelectedComponent != NULL)
   {
      Box3F boundingBox = mSelectedComponent->getBoundingBox();
      boundingBox.transform(mSelectedObject->mTransformMatrix);

      // Bounding Box
      Plugins::Link.Graphics.drawBox3D(mProjectManager->mRenderLayer4View->id, boundingBox, ColorI(0, 255, 0, 255), NULL);

      // Render Gizmo
      mGizmo.render();
   }
}

bool SceneTool::onMouseLeftDown(int x, int y)
{
   Point3F worldRay = Plugins::Link.Rendering.screenToWorld(Point2I(x, y));
   Point3F editorPos = Plugins::Link.Scene.getActiveCamera()->getPosition();

   if (!mGizmo.onMouseLeftDown(x, y))
   {
      Scene::SceneObject* hit = Plugins::Link.Scene.raycast(editorPos, editorPos + (worldRay * 1000.0f));
      if (mSelectedObject != hit)
      {
         if (hit)
            selectObject(hit, true);
      }
   }

   return false;
}

bool SceneTool::onMouseLeftUp(int x, int y)
{
   mGizmo.onMouseLeftUp(x, y);

   if ( mSelectedObject != NULL )
      loadObjectProperties(mScenePanel->propertyGrid, mSelectedObject);

   return false;
}

bool SceneTool::onMouseMove(int x, int y)
{
   mGizmo.onMouseMove(x, y);
   return false;
}

void SceneTool::onSceneChanged()
{
   refreshObjectList();
   refreshFeatureList();
   refreshChoices();
}

void SceneTool::onProjectLoaded(const wxString& projectName, const wxString& projectPath)
{
   refreshObjectList();
   refreshFeatureList();
   refreshChoices();

   if ( mLightIcon == NULL )
      mLightIcon = Plugins::Link.Graphics.loadTexture("light.png", TextureHandle::BitmapKeepTexture, BGFX_TEXTURE_NONE, false, false);
}

void SceneTool::onProjectClosed()
{

}

void SceneTool::OnMenuEvent(wxCommandEvent& evt)
{
   if (evt.GetId() == SCENE_NEW)
      Plugins::Link.Scene.clear();

   if (evt.GetId() == SCENE_OPEN)
   {
      wxFileDialog openFile(mFrame, wxT("Open Scene File"), "", "", "taml files (*.taml)|*.taml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
      if (openFile.ShowModal() == wxID_OK)
         Plugins::Link.Scene.load(openFile.GetPath());
   }

   if (evt.GetId() == SCENE_SAVE)
   {
      wxFileDialog saveFile(mFrame, wxT("Save Scene File"), "", "", "taml files (*.taml)|*.taml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (saveFile.ShowModal() == wxID_OK)
         Plugins::Link.Scene.save(saveFile.GetPath());
   }

   if (evt.GetId() == ADD_FEATURE_BUTTON)
      openAddFeatureMenu();

   if (evt.GetId() == ADD_OBJECT_BUTTON)
      openAddObjectMenu();

   if (evt.GetId() == ADD_COMPONENT_BUTTON)
      openAddComponentMenu();

   refreshObjectList();
   refreshFeatureList();
   refreshChoices();
}

void SceneTool::openAddObjectMenu()
{
   wxMenu* menu = new wxMenu;
   menu->Append(0, wxT("Empty Object"));

   // From Asset
   wxMenu* fromAssetMenu         = new wxMenu;
   Vector<ModuleInfo>* modules   = mProjectManager->getModuleList();
   const char* currentModuleID   = "";
   wxMenu* currentModuleMenu     = NULL;
   U32 menuItemID                = 1;
   for (Vector<ModuleInfo>::iterator modulesItr = modules->begin(); modulesItr != modules->end(); ++modulesItr)
   {
      if (dStrcmp(modulesItr->moduleID, currentModuleID) != 0)
      {
         currentModuleID = modulesItr->moduleID;
         currentModuleMenu = new wxMenu;
      }

      for (Vector<AssetCategoryInfo>::iterator assetCatItr = modulesItr->assets.begin(); assetCatItr != modulesItr->assets.end(); ++assetCatItr)
      {
         wxMenu* currentCategoryMenu = new wxMenu;
         for (Vector<const AssetDefinition*>::iterator assetItr = assetCatItr->assets.begin(); assetItr != assetCatItr->assets.end(); ++assetItr)
         {
            // Fetch asset definition.
            const AssetDefinition* pAssetDefinition = *assetItr;
            char buf[256];
            dStrcpy(buf, pAssetDefinition->mAssetId);
            const char* moduleName  = dStrtok(buf, ":");
            const char* assetName   = dStrtok(NULL, ":");
            currentCategoryMenu->Append(menuItemID, assetName);
            menuItemID++;
         }
         currentModuleMenu->AppendSubMenu(currentCategoryMenu, assetCatItr->categoryName);
      }

      fromAssetMenu->AppendSubMenu(currentModuleMenu, modulesItr->moduleID);
   }
   menu->AppendSubMenu(fromAssetMenu, wxT("From Asset"));

   menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnAddObjectMenuEvent), NULL, this);
   mFrame->PopupMenu(menu, wxDefaultPosition);
   delete menu;
}

void SceneTool::OnAddObjectMenuEvent(wxCommandEvent& evt)
{
   if (evt.GetId() == 0)
   {
      Scene::SceneObject* newObject = new Scene::SceneObject();
      newObject->registerObject("NewSceneObject");
      Plugins::Link.Scene.addObject(newObject, "NewSceneObject");
      refreshObjectList();
   }

   if (evt.GetId() > 0)
   {
      Vector<ModuleInfo>* modules = mProjectManager->getModuleList();
      U32 menuItemID = 1;
      for (Vector<ModuleInfo>::iterator modulesItr = modules->begin(); modulesItr != modules->end(); ++modulesItr)
      {
         for (Vector<AssetCategoryInfo>::iterator assetCatItr = modulesItr->assets.begin(); assetCatItr != modulesItr->assets.end(); ++assetCatItr)
         {
            for (Vector<const AssetDefinition*>::iterator assetItr = assetCatItr->assets.begin(); assetItr != assetCatItr->assets.end(); ++assetItr)
            {
               // Fetch asset definition.
               const AssetDefinition* pAssetDefinition = *assetItr;
               if (evt.GetId() == menuItemID)
               {
                  Point3F editorPos = Plugins::Link.Scene.getActiveCamera()->getPosition();
                  if ( dStrcmp(pAssetDefinition->mAssetType, "ObjectTemplateAsset") == 0 )
                     mProjectManager->addObjectTemplateAsset(pAssetDefinition->mAssetId, editorPos);
                  else if (dStrcmp(pAssetDefinition->mAssetType, "MeshAsset") == 0)
                     mProjectManager->addMeshAsset(pAssetDefinition->mAssetId, editorPos);
                  return;
               }
               menuItemID++;
            }
         }
      }
   }
}

void SceneTool::openAddComponentMenu()
{
   wxMenu* menu = new wxMenu;

   refreshClassLists();
   for (S32 i = 0; i < mComponentClassList.size(); ++i)
      menu->Append(i, mComponentClassList[i]);

   menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnAddComponentMenuEvent), NULL, this);
   mFrame->PopupMenu(menu, wxDefaultPosition);
   delete menu;
}

void SceneTool::OnAddComponentMenuEvent(wxCommandEvent& evt)
{
   addComponent(mSelectedObject, mComponentClassList[evt.GetId()]);
}

void SceneTool::openAddFeatureMenu()
{
   wxMenu* menu = new wxMenu;

   refreshClassLists();

   for (S32 i = 0; i < mFeatureClassList.size(); ++i)
      menu->Append(i, mFeatureClassList[i]);

   menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnAddFeatureMenuEvent), NULL, this);
   mFrame->PopupMenu(menu, wxDefaultPosition);
   delete menu;
}

void SceneTool::OnAddFeatureMenuEvent(wxCommandEvent& evt)
{
   Scene::SceneFeature* feature = dynamic_cast<Scene::SceneFeature*>(Plugins::Link.Con.createObject(mFeatureClassList[evt.GetId()]));
   if (feature)
   {
      feature->registerObject();
      Plugins::Link.Scene.addFeature(feature);
      refreshFeatureList();
   }
}

void SceneTool::OnTreeEvent( wxTreeEvent& evt )
{
   if (mRefreshing)
      return;

   if (evt.GetId() == OBJECT_LIST)
   {
      ObjectTreeItemData* data = dynamic_cast<ObjectTreeItemData*>(mScenePanel->objectList->GetItemData(evt.GetItem()));
      if (data)
      {
         // Did we select an object?
         Scene::SceneObject* Object = dynamic_cast<Scene::SceneObject*>(data->objPtr);
         if (Object)
         {
            selectObject(Object);
            return;
         }

         // or a component?
         Scene::BaseComponent* component = dynamic_cast<Scene::BaseComponent*>(data->objPtr);
         if (component)
         {
            selectComponent(component);
            return;
         }
      }
   }

   if (evt.GetId() == FEATURE_LIST)
   {
      FeatureTreeItemData* data = dynamic_cast<FeatureTreeItemData*>(mScenePanel->featureList->GetItemData(evt.GetItem()));
      if (data)
      {
         // Did we select a feature?
         Scene::SceneFeature* feature = dynamic_cast<Scene::SceneFeature*>(data->objPtr);
         if (feature)
         {
            mSelectedFeature = feature;
            loadObjectProperties(mScenePanel->featurePropGrid, feature);
            return;
         }
      }
   }
}

void SceneTool::OnTreeMenu( wxTreeEvent& evt ) 
{ 
   if (evt.GetId() == OBJECT_LIST)
   {
      ObjectTreeItemData* data = dynamic_cast<ObjectTreeItemData*>(mScenePanel->objectList->GetItemData(evt.GetItem()));

      Scene::SceneObject* Object = dynamic_cast<Scene::SceneObject*>(data->objPtr);
      if (Object)
      {
         mMenuObject = Object;

         wxMenu* menu = new wxMenu;
         menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnObjectMenuEvent), NULL, this);
         menu->Append(0, wxT("Delete Object"));

         // Find all components in console namespace list then make a menu.
         wxMenu* compMenu = new wxMenu;
         refreshClassLists();
         for (S32 i = 0; i < mComponentClassList.size(); ++i)
            compMenu->Append(1 + i, mComponentClassList[i]);
         menu->AppendSubMenu(compMenu, wxT("Add Component"));

         mFrame->PopupMenu(menu, wxDefaultPosition);
         delete menu;

         mMenuObject = NULL;
         return;
      }

      Scene::BaseComponent* component = dynamic_cast<Scene::BaseComponent*>(data->objPtr);
      if (component)
      {
         mMenuComponent = component;

         wxMenu* menu = new wxMenu;
         menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnComponentMenuEvent), NULL, this);
         menu->Append(0, wxT("Delete Component"));
         mFrame->PopupMenu(menu, wxDefaultPosition);
         delete menu;

         mMenuComponent = NULL;
         return;
      }
   }

   if (evt.GetId() == FEATURE_LIST)
   {
      FeatureTreeItemData* data = dynamic_cast<FeatureTreeItemData*>(mScenePanel->featureList->GetItemData(evt.GetItem()));
      if (data)
      {
         Scene::SceneFeature* feature = dynamic_cast<Scene::SceneFeature*>(data->objPtr);
         if (feature)
         {
            mMenuFeature = feature;

            wxMenu* menu = new wxMenu;
            menu->Connect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SceneTool::OnFeatureMenuEvent), NULL, this);
            menu->Append(0, wxT("Delete Feature"));

            mFrame->PopupMenu(menu, wxDefaultPosition);
            delete menu;

            mMenuFeature = NULL;
         }
      }
   }
} 

void SceneTool::OnObjectMenuEvent(wxCommandEvent& evt)
{
   // Delete Object
   if (evt.GetId() == 0)
   {
      if (mSelectedObject == mMenuObject)
      {
         mSelectedObject = NULL;
         mSelectedComponent = NULL;
         mScenePanel->addComponentButton->Enable(false);
      }

      Plugins::Link.Scene.deleteObject(mMenuObject);
      refreshObjectList();
      return;
   }

   // Add New Component
   if (evt.GetId() >= 1)
      addComponent(mMenuObject, mComponentClassList[evt.GetId() - 1]);
}

void SceneTool::OnComponentMenuEvent(wxCommandEvent& evt)
{
   // Delete Object
   if (evt.GetId() == 0)
   {
      // Sanity check. mOwnerObject should never be NULL.
      if (mMenuComponent->mOwnerObject != NULL)
      {
         mMenuComponent->mOwnerObject->removeComponent(mMenuComponent);
         mMenuComponent = NULL;
         refreshObjectList();
         return;
      }
   }
}

void SceneTool::OnFeatureMenuEvent(wxCommandEvent& evt)
{
   // Delete Object
   if (evt.GetId() == 0)
   {
      Plugins::Link.Scene.deleteFeature(mMenuFeature);
      refreshFeatureList();
      return;
   }
}

void SceneTool::OnObjectPropChanged(wxPropertyGridEvent& evt)
{
   wxString name = evt.GetPropertyName();
   wxVariant val = evt.GetPropertyValue();
   wxString strVal = val.GetString();

   // Special Name Handling.
   if (name == "Name" && mSelectedObject)
   {
      mSelectedObject->assignUniqueName(strVal);
      refreshObjectList();
      return;
   }
   else if (name == "InternalName" && mSelectedComponent)
   {
      mSelectedComponent->setInternalName(strVal);
      refreshObjectList();
      return;
   }

   // Special Field Handling by Name
   if (name == "MeshAsset")
   {
      long intVal = val.GetInteger();
      strVal = mMeshChoices.GetLabel(intVal);
   }
   else if (name.StartsWith("MaterialAsset"))
   {
      long intVal = val.GetInteger();
      strVal = mMaterialChoices.GetLabel(intVal);
   }
   else if (name.StartsWith("SubMesh"))
   {
      strVal = val.GetBool() ? "true" : "false";
   }

   // Special Field Handling by Type
   U32 type = mSelectedObject->getDataFieldType(Plugins::Link.StringTableLink->insert(name), NULL);
   if (type == Plugins::Link.Con.TypeColorF)
   {
      wxColour color;
      color << val;
      strVal.Printf("%f %f %f 1.0", color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f);
   }

   // Assign the value and refresh the Object. 
   // Note: No need to refresh a selected component, better to refresh the whole Object.
   mSelectedObject->setDataField(Plugins::Link.StringTableLink->insert(name), NULL, strVal);
   mSelectedObject->refresh();
}

void SceneTool::OnFeaturePropChanged(wxPropertyGridEvent& evt)
{
   wxString name = evt.GetPropertyName();
   wxVariant val = evt.GetPropertyValue();
   wxString strVal = val.GetString();

   // Check field type.
   U32 type = mSelectedFeature->getDataFieldType(Plugins::Link.StringTableLink->insert(name), NULL);
   if (type == Plugins::Link.Con.TypeColorF)
   {
      wxColour color;
      color << val;
      strVal.Printf("%f %f %f 1.0", color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f);
   }

   mSelectedFeature->setDataField(Plugins::Link.StringTableLink->insert(name), NULL, strVal);
   mSelectedFeature->refresh();
}

void SceneTool::refreshObjectList()
{
   if (!mProjectManager->isProjectLoaded())
      return;

   mRefreshing = true;

   // Clear list.
   mScenePanel->objectList->DeleteAllItems();
   mObjectListRoot = mScenePanel->objectList->AddRoot("ROOT");

   wxTreeItemId selectItem = mObjectListRoot;
   SimGroup* sceneGroup = Plugins::Link.Scene.getSceneGroup();
   if ( sceneGroup != NULL )
   {
      for(S32 n = 0; n < sceneGroup->size(); ++n)
      {
         Scene::SceneObject* Object = dynamic_cast<Scene::SceneObject*>(sceneGroup->at(n));
         if ( !Object ) continue;
         wxTreeItemId ObjectItem = mScenePanel->objectList->AppendItem(mObjectListRoot, Object->getName(), 0, -1, new ObjectTreeItemData(Object));
         if (mSelectedObject == Object)
            selectItem = ObjectItem;

         // Iterate components
         for(S32 n = 0; n < Object->mComponents.size(); ++n)
         {
            Scene::BaseComponent* component = Object->mComponents[n];
            if (!component) continue;

            wxString compName(component->getClassName());
            const char* internalName = component->getInternalName();
            if (internalName)
            {
               compName = internalName;
               compName.Append(" (");
               compName.Append(component->getClassName());
               compName.Append(")");
            }

            wxTreeItemId componentItem = mScenePanel->objectList->AppendItem(ObjectItem, compName, 1, -1, new ObjectTreeItemData(component));
            if (mSelectedComponent == component)
               selectItem = componentItem;
         }
      }
   }

   // Retain selection.
   if (selectItem != mObjectListRoot)
      mScenePanel->objectList->SelectItem(selectItem);

   mScenePanel->objectList->Refresh();
   mRefreshing = false;
}

void SceneTool::refreshFeatureList()
{
   if (!mProjectManager->isProjectLoaded())
      return;

   mRefreshing = true;

   // Clear list.
   mScenePanel->featureList->DeleteAllItems();
   mObjectListRoot = mScenePanel->featureList->AddRoot("ROOT");

   wxTreeItemId selectItem = mFeatureListRoot;
   SimGroup* featureGroup = Plugins::Link.Scene.getSceneGroup();
   if (featureGroup != NULL)
   {
      for (S32 n = 0; n < featureGroup->size(); ++n)
      {
         Scene::SceneFeature* feature = dynamic_cast<Scene::SceneFeature*>(featureGroup->at(n));
         if (!feature) continue;
         wxTreeItemId featureItem = mScenePanel->featureList->AppendItem(mFeatureListRoot, feature->mName, 0, -1, new FeatureTreeItemData(feature));
         if (mSelectedFeature == feature)
            selectItem = featureItem;
      }
   }

   // Retain selection.
   if (selectItem != mFeatureListRoot)
      mScenePanel->featureList->SelectItem(selectItem);

   mScenePanel->featureList->Refresh();
   mRefreshing = false;
}

void SceneTool::refreshChoices()
{
   if (!mProjectManager->isProjectLoaded())
      return;

   mRefreshing = true;
   mMaterialChoices.Clear();
   mMeshChoices.Clear();
   mObjectTemplateChoices.Clear();
   
   Vector<const AssetDefinition*> assetDefinitions = Plugins::Link.AssetDatabaseLink.getDeclaredAssets();

   // Iterate sorted asset definitions.
   for (Vector<const AssetDefinition*>::iterator assetItr = assetDefinitions.begin(); assetItr != assetDefinitions.end(); ++assetItr)
   {
      // Fetch asset definition.
      const AssetDefinition* pAssetDefinition = *assetItr;

      //char buf[256];
      //dStrcpy(buf, pAssetDefinition->mAssetId);
      //const char* moduleName = dStrtok(buf, ":");
      //const char* assetName = dStrtok(NULL, ":");

      // Populate Material choices menu.
      if (dStrcmp(pAssetDefinition->mAssetType, "MaterialAsset") == 0)
         mMaterialChoices.Add(pAssetDefinition->mAssetId, mMaterialChoices.GetCount());

      // Populate Mesh choices menu.
      if (dStrcmp(pAssetDefinition->mAssetType, "MeshAsset") == 0)
         mMeshChoices.Add(pAssetDefinition->mAssetId, mMeshChoices.GetCount());

      // Object Templates
      if (dStrcmp(pAssetDefinition->mAssetType, "ObjectTemplateAsset") == 0)
         mObjectTemplateChoices.Add(pAssetDefinition->mAssetId, mMeshChoices.GetCount());
   }

   mRefreshing = false;
}

void SceneTool::refreshClassLists()
{
   mComponentClassList.clear();
   mFeatureClassList.clear();

   for (Namespace *walk = Plugins::Link.Con.getNamespaceList(); walk; walk = walk->mNext)
   {
      if (walk->mParent == NULL)
         continue;

      if (dStrcmp(walk->mParent->mName, "BaseComponent") == 0)
         mComponentClassList.push_back(walk->mName);

      if (dStrcmp(walk->mParent->mName, "SceneFeature") == 0 ||
          dStrcmp(walk->mParent->mName, "RenderFeature") == 0 ||
          dStrcmp(walk->mParent->mName, "PostRenderFeature") == 0)
      {
         if (dStrcmp(walk->mName, "RenderFeature") == 0 || dStrcmp(walk->mName, "PostRenderFeature") == 0)
            continue;

         mFeatureClassList.push_back(walk->mName);
      }
   }
}

static S32 QSORT_CALLBACK compareEntries(const void* a, const void* b)
{
   SimFieldDictionary::Entry *fa = *((SimFieldDictionary::Entry **)a);
   SimFieldDictionary::Entry *fb = *((SimFieldDictionary::Entry **)b);
   return dStricmp(fa->slotName, fb->slotName);
}

void SceneTool::loadObjectProperties(wxPropertyGrid* propertyGrid, SimObject* obj)
{
   propertyGrid->Clear();

   Scene::SceneObject* Object = dynamic_cast<Scene::SceneObject*>(obj);
   if (Object)
   {
      propertyGrid->Append(new wxPropertyCategory("SceneObject"));
      propertyGrid->Append(new wxStringProperty("Name", "Name", Object->getName()));
   }

   Scene::BaseComponent* component = dynamic_cast<Scene::BaseComponent*>(obj);
   if (component)
   {
      const char* internalName = component->getInternalName();
      propertyGrid->Append(new wxPropertyCategory("BaseComponent"));
      propertyGrid->Append(new wxStringProperty("InternalName", "InternalName", internalName));
   }

   // Add static fields.
   wxString fieldGroup("");
   bool addFieldGroup = false;
   AbstractClassRep::FieldList fieldList = obj->getFieldList();
   for(Vector<AbstractClassRep::Field>::iterator itr = fieldList.begin(); itr != fieldList.end(); itr++)
   {
      const AbstractClassRep::Field* f = itr;
      if( f->type == AbstractClassRep::DepricatedFieldType ||
            f->type == AbstractClassRep::EndGroupFieldType) 
         continue;

      if ( f->type == AbstractClassRep::StartGroupFieldType )
      {
         addFieldGroup = true;
         fieldGroup = f->pGroupname;
         continue;
      }

      for (U32 j = 0; S32(j) < f->elementCount; j++)
      {
         const char *val = (*f->getDataFn)(obj, Plugins::Link.Con.getData(f->type, (void *)(((const char *)obj) + f->offset), j, f->table, f->flag));

         if (!val)
            continue;

         if (addFieldGroup)
         {
            propertyGrid->Append(new wxPropertyCategory(fieldGroup));
            addFieldGroup = false;
         }

         if (dStrcmp(f->pFieldname, "MeshAsset") == 0)
            propertyGrid->Append(new wxEnumProperty("MeshAsset", wxPG_LABEL, mMeshChoices));
         else if (f->type == Plugins::Link.Con.TypeColorF)
         {
            ColorF colorVal;
            Plugins::Link.Con.setData(Plugins::Link.Con.TypeColorF, &colorVal, 0, 1, &val, NULL, 0);
            wxColour color;
            color.Set(colorVal.red * 255, colorVal.green * 255, colorVal.blue * 255, 255);
            propertyGrid->Append(new wxColourProperty(f->pFieldname, f->pFieldname, color));
         }
         else if (f->type == Plugins::Link.Con.TypeBool)
         {
            bool boolVal;
            Plugins::Link.Con.setData(Plugins::Link.Con.TypeBool, &boolVal, 0, 1, &val, NULL, 0);
            propertyGrid->Append(new wxBoolProperty(f->pFieldname, f->pFieldname, boolVal));
         }
         else
            propertyGrid->Append(new wxStringProperty(f->pFieldname, f->pFieldname, val));
      }
   }

   // Get list of dynamic fields and sort by name
   Vector<SimFieldDictionary::Entry *> flist;
   SimFieldDictionary* fieldDictionary = obj->getFieldDictionary();
   for (SimFieldDictionaryIterator ditr(fieldDictionary); *ditr; ++ditr)
      flist.push_back(*ditr);
   dQsort(flist.address(), flist.size(), sizeof(SimFieldDictionary::Entry *), compareEntries);

   // Add dynamic fields.
   wxPGProperty* materialsCategory = NULL;
   wxPGProperty* submeshCategory = NULL;
   wxPGProperty* otherCategory = NULL;
   for (U32 i = 0; i < (U32)flist.size(); i++)
   {
      SimFieldDictionary::Entry* entry = flist[i];

      if (dStrncmp(entry->slotName, "MaterialAsset", 13) == 0)
      {
         if (materialsCategory == NULL)
            materialsCategory = propertyGrid->Append(new wxPropertyCategory("Materials"));
         propertyGrid->Append(new wxEnumProperty(entry->slotName, wxPG_LABEL, mMaterialChoices));
      }
      else if (dStrncmp(entry->slotName, "SubMesh", 7) == 0)
      {
         if (submeshCategory == NULL)
            submeshCategory = propertyGrid->Append(new wxPropertyCategory("SubMeshes"));
         propertyGrid->AppendIn(submeshCategory, new wxBoolProperty(entry->slotName, wxPG_LABEL, dAtob(entry->value)));
      }
      else
      {
         if (otherCategory == NULL)
            otherCategory = propertyGrid->Append(new wxPropertyCategory("Other"));
         propertyGrid->AppendIn(otherCategory, new wxStringProperty(entry->slotName, entry->slotName, entry->value));
      }
   }
}

void SceneTool::selectObject(Scene::SceneObject* obj, bool updateTree)
{
   mSelectedObject = obj;
   mSelectedComponent = NULL;
   mScenePanel->addComponentButton->Enable(true);

   // Update the gizmo.
   mGizmo.selectObject(obj);

   // Update the tree selection.
   if (updateTree)
   {
      wxTreeItemIdValue cookie;
      wxTreeItemId item = mScenePanel->objectList->GetFirstChild(mObjectListRoot, cookie);
      wxTreeItemId child;
      while (item.IsOk())
      {
         ObjectTreeItemData* data = dynamic_cast<ObjectTreeItemData*>(mScenePanel->objectList->GetItemData(item));
         if (data)
         {
            Scene::SceneObject* dataObject = dynamic_cast<Scene::SceneObject*>(data->objPtr);
            if (dataObject == obj)
            {
               mScenePanel->objectList->SelectItem(item);
               break;
            }
         }
         item = mScenePanel->objectList->GetNextChild(mObjectListRoot, cookie);
      }
   }

   // Property Grid
   loadObjectProperties(mScenePanel->propertyGrid, obj);
}

void SceneTool::addComponent(Scene::SceneObject* Object, StringTableEntry componentClassName)
{
   Scene::BaseComponent* newComponent = dynamic_cast<Scene::BaseComponent*>(Plugins::Link.Con.createObject(componentClassName));
   if (newComponent)
   {
      newComponent->registerObject();
      Object->addComponent(newComponent);
      Object->refresh();
      refreshObjectList();
      selectComponent(newComponent, true);
   }
}

void SceneTool::selectComponent(Scene::BaseComponent* component, bool updateTree)
{
   mSelectedObject = component->mOwnerObject;
   mSelectedComponent = component;

   // Update the gizmo.
   mGizmo.selectComponent(component);

   // Update the tree selection.
   if (updateTree)
   {
      wxTreeItemIdValue cookie;
      wxTreeItemId item = mScenePanel->objectList->GetFirstChild(mObjectListRoot, cookie);
      wxTreeItemId child;
      while (item.IsOk())
      {
         ObjectTreeItemData* data = dynamic_cast<ObjectTreeItemData*>(mScenePanel->objectList->GetItemData(item));
         if (data)
         {
            Scene::BaseComponent* dataComponent = dynamic_cast<Scene::BaseComponent*>(data->objPtr);
            if (dataComponent == component)
            {
               mScenePanel->objectList->SelectItem(item);
               break;
            }
         }
         item = mScenePanel->objectList->GetNextChild(mObjectListRoot, cookie);
      }
   }

   // Property Grid
   loadObjectProperties(mScenePanel->propertyGrid, component);
}

void SceneTool::OnToolbarDropdownEvent(wxCommandEvent& evt)
{
   switch (evt.GetId())
   {
      case 1:
         mFrame->PopupMenu(mTranslateMenu, wxDefaultPosition);
         break;

      case 2:
         mFrame->PopupMenu(mRotateMenu, wxDefaultPosition);
         break;

      case 3:
         mFrame->PopupMenu(mScaleMenu, wxDefaultPosition);
         break;

      default:
         break;
   }
}

void SceneTool::OnTranslateMenuEvent(wxCommandEvent& evt)
{
   switch (evt.GetId())
   {
      case 0:
         mGizmo.mTranslateSnap = 0.0f;
         break;

      case 1:
         mGizmo.mTranslateSnap = 0.1f;
         break;

      case 2:
         mGizmo.mTranslateSnap = 0.5f;
         break;

      case 3:
         mGizmo.mTranslateSnap = 1.0f;
         break;

      case 4:
         mGizmo.mTranslateSnap = 5.0f;
         break;
   }
}

void SceneTool::OnRotateMenuEvent(wxCommandEvent& evt)
{
   switch (evt.GetId())
   {
      case 0:
         mGizmo.mRotateSnap = 0.0f;
         break;

      case 1:
         mGizmo.mRotateSnap = M_PI_F / 36.0f;
         break;

      case 2:
         mGizmo.mRotateSnap = M_PI_F / 12.0f;
         break;

      case 3:
         mGizmo.mRotateSnap = M_PI_F / 4.0f;
         break;

      case 4:
         mGizmo.mRotateSnap = M_PI_F / 2.0f;
         break;
   }
}

void SceneTool::OnScaleMenuEvent(wxCommandEvent& evt)
{
   switch (evt.GetId())
   {
      case 0:
         mGizmo.mScaleSnap = 0.0f;
         break;

      case 1:
         mGizmo.mScaleSnap = 0.1f;
         break;

      case 2:
         mGizmo.mScaleSnap = 0.5f;
         break;

      case 3:
         mGizmo.mScaleSnap = 1.0f;
         break;

      case 4:
         mGizmo.mScaleSnap = 5.0f;
         break;   
   }
}