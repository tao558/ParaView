/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGroupInputsWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVGroupInputsWidget.h"

#include "vtkKWWidget.h"
#include "vtkKWListBox.h"
#include "vtkKWLabel.h"
#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVWindow.h"
#include "vtkPVSourceCollection.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGroupInputsWidget);
vtkCxxRevisionMacro(vtkPVGroupInputsWidget, "1.4");

int vtkPVGroupInputsWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGroupInputsWidget::vtkPVGroupInputsWidget()
{
  this->CommandFunction = vtkPVGroupInputsWidgetCommand;
  
  this->PartSelectionList = vtkKWListBox::New();
  this->PartLabelCollection = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVGroupInputsWidget::~vtkPVGroupInputsWidget()
{
  this->PartSelectionList->Delete();
  this->PartSelectionList = NULL;
  this->PartLabelCollection->Delete();
  this->PartLabelCollection = NULL;
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Create(vtkKWApplication *app)
{
  if (this->Application)
    {
    vtkErrorMacro("PVWidget already created");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->ScrollbarOff();
  this->PartSelectionList->Create(app, "-selectmode extended");
  this->PartSelectionList->SetHeight(0);
  // I assume we need focus for control and alt modifiers.
  this->Script("bind %s <Enter> {focus %s}",
               this->PartSelectionList->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  this->Script("pack %s -side top -fill both -expand t",
               this->PartSelectionList->GetWidgetName());

  // There is no current way to get a modified call back, so assume
  // the user will change the list.  This widget will only be used once anyway.
  this->ModifiedFlag = 1;
}


//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Inactivate()
{
  int num, idx;
  vtkKWLabel* label;

  this->Script("pack forget %s", this->PartSelectionList->GetWidgetName());

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    if (this->PartSelectionList->GetSelectState(idx))
      {
      label = vtkKWLabel::New();
      label->SetParent(this);
      label->SetLabel(this->PartSelectionList->GetItem(idx));
      label->Create(this->Application, "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->PartLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::AcceptInternal(const char* vtkSourceTclName)
{
  int numParts, partIdx;
  int num, idx, count;
  int state;
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVData *pvd;
  vtkPVPart *part;

  pvWin = this->PVSource->GetPVWindow();
  sources = pvWin->GetSourceList("Sources");
  sources->InitTraversal();

  num = this->PartSelectionList->GetNumberOfItems();
  count = 0;

  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {
    this->Inactivate();
    }

  // Now loop through the input mask setting the selection states.
  pvApp->BroadcastScript("%s RemoveAllInputs",
                         vtkSourceTclName);
  this->PVSource->RemoveAllPVInputs();  
  sources->InitTraversal();
  for (idx = 0; idx < num; ++idx)
    {
    pvs = sources->GetNextPVSource();
    if (pvs == NULL)
      { // sanity check.
      vtkErrorMacro("Source mismatch.");
      return;
      }
    pvd = pvs->GetPVOutput();
    state = this->PartSelectionList->GetSelectState(idx);
    if (state)
      {
      this->PVSource->SetPVInput(count++, pvd);
      // Special replace input feature.
      // Visibility of ALL selected input turned off.
      pvs->SetVisibility(0);
      numParts = pvd->GetNumberOfPVParts();
      for (partIdx = 0; partIdx < numParts; ++partIdx)
        {
        part = pvd->GetPVPart(partIdx);
        pvApp->BroadcastScript("%s AddInput %s",  vtkSourceTclName,
                               part->GetVTKDataTclName());
        }
      }
    }

  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVGroupInputsWidget::SetSelectState(int idx, int val)
{
  this->PartSelectionList->SetSelectState(idx, val);
}


//---------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Trace(ofstream *file)
{
  int idx, num;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << idx << " " << this->PartSelectionList->GetSelectState(idx) << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::ResetInternal(const char* vtkSourceTclName)
{
  int idx;
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;

  pvWin = this->PVSource->GetPVWindow();
  sources = pvWin->GetSourceList("Sources");

  this->PartSelectionList->DeleteAll();
  idx = 0;
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    this->PartSelectionList->InsertEntry(idx, pvs->GetName());
    ++idx;
    }

  // Set visible inputs to selected.
  idx = 0;
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (pvs->GetVisibility())
      {
      this->PartSelectionList->SetSelectState(idx, 1);
      }
    ++idx;
    }

  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::SaveInBatchScript(ofstream *file)
{
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
