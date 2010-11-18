/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QInputDialog>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerColorsModuleWidget.h"
#include "ui_qSlicerColorsModule.h"

// MRML includes
#include <vtkMRMLColorNode.h>
#include <vtkMRMLColorTableNode.h>
#include <vtkMRMLProceduralColorNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>

//-----------------------------------------------------------------------------
class qSlicerColorsModuleWidgetPrivate: public Ui_qSlicerColorsModule
{
public:
  qSlicerColorsModuleWidgetPrivate();
  virtual ~qSlicerColorsModuleWidgetPrivate();
  vtkScalarBarWidget* ScalarBarWidget;
};

qSlicerColorsModuleWidgetPrivate::qSlicerColorsModuleWidgetPrivate()
{
  this->ScalarBarWidget = vtkScalarBarWidget::New();
  this->ScalarBarWidget->GetScalarBarActor()->SetOrientationToVertical();
  this->ScalarBarWidget->GetScalarBarActor()->SetNumberOfLabels(11);
  this->ScalarBarWidget->GetScalarBarActor()->SetTitle("(mm)");
  this->ScalarBarWidget->GetScalarBarActor()->SetLabelFormat(" %#8.3f");
  
  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarWidget->GetScalarBarActor()->SetPosition(0.1, 0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetWidth(0.1);
  this->ScalarBarWidget->GetScalarBarActor()->SetHeight(0.8);
}

//-----------------------------------------------------------------------------
qSlicerColorsModuleWidgetPrivate::~qSlicerColorsModuleWidgetPrivate()
{
  if (this->ScalarBarWidget)
    {
    this->ScalarBarWidget->Delete();
    this->ScalarBarWidget = 0;
    }
}

//-----------------------------------------------------------------------------
qSlicerColorsModuleWidget::qSlicerColorsModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerColorsModuleWidgetPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerColorsModuleWidget::~qSlicerColorsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerColorsModuleWidget::setup()
{
  Q_D(qSlicerColorsModuleWidget);
  d->setupUi(this);
  QIcon copyIcon = this->style()->standardIcon(QStyle::SP_FileIcon);
  d->CopyColorNodeButton->setIcon(copyIcon);
  vtkRenderer* activeRenderer = qSlicerApplication::application()->layoutManager()->activeThreeDRenderer();
  if (activeRenderer)
    {
    d->ScalarBarWidget->SetInteractor(activeRenderer->GetRenderWindow()->GetInteractor());
    }
  d->VTKScalarBar->setScalarBarWidget(d->ScalarBarWidget);
  connect(d->ColorTableComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(onMRMLColorNodeChanged(vtkMRMLNode*)));
  connect(d->NumberOfColorsSpinBox, SIGNAL(valueChanged(int)),
          this, SLOT(setNumberOfColors(int)));
  connect(d->LUTRangeWidget, SIGNAL(valuesChanged(double, double)),
          this, SLOT(setLookupTableRange(double, double)));
  connect(d->CopyColorNodeButton, SIGNAL(clicked()),
          this, SLOT(copyCurrentColorNode()));
}

//-----------------------------------------------------------------------------
void qSlicerColorsModuleWidget::onMRMLColorNodeChanged(vtkMRMLNode* newColorNode)
{
  Q_D(qSlicerColorsModuleWidget);
  vtkMRMLColorNode* colorNode = vtkMRMLColorNode::SafeDownCast(newColorNode);
  if (!colorNode)
    {
    d->NumberOfColorsSpinBox->setEnabled(false);
    d->LUTRangeWidget->setEnabled(false);
    d->CopyColorNodeButton->setEnabled(false);
    return;
    }
  d->CopyColorNodeButton->setEnabled(true);
  d->NumberOfColorsSpinBox->setEnabled(
    colorNode->GetType() == vtkMRMLColorTableNode::User);
  d->NumberOfColorsSpinBox->setValue(colorNode->GetNumberOfColors());
  Q_ASSERT(d->NumberOfColorsSpinBox->value() == colorNode->GetNumberOfColors());

  if (colorNode->GetLookupTable())
    {
    d->LUTRangeWidget->setEnabled(true);
    double* range = colorNode->GetLookupTable()->GetRange();
    d->LUTRangeWidget->setValues(range[0], range[1]);
    d->ScalarBarWidget->GetScalarBarActor()->SetLookupTable(colorNode->GetLookupTable());
    }
  else
    {
    d->LUTRangeWidget->setEnabled(false);
    d->LUTRangeWidget->setValues(0.,0.);
    vtkMRMLProceduralColorNode *procColorNode = vtkMRMLProceduralColorNode::SafeDownCast(colorNode);
    if (procColorNode)
      {
      d->ScalarBarWidget->GetScalarBarActor()->SetLookupTable(procColorNode->GetColorTransferFunction());
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerColorsModuleWidget::setNumberOfColors(int newNumber)
{
  Q_D(qSlicerColorsModuleWidget);
  if (!d->NumberOfColorsSpinBox->isEnabled())
    {
    return;
    }
  vtkMRMLColorTableNode* colorTableNode = vtkMRMLColorTableNode::SafeDownCast(
    d->ColorTableComboBox->currentNode());
  colorTableNode->SetNumberOfColors(newNumber);
}

//-----------------------------------------------------------------------------
void qSlicerColorsModuleWidget::setLookupTableRange(double min, double max)
{
  Q_D(qSlicerColorsModuleWidget);
  if (!d->LUTRangeWidget->isEnabled())
    {
    return;
    }
  vtkMRMLColorNode* colorNode = vtkMRMLColorNode::SafeDownCast(d->ColorTableComboBox->currentNode());
  Q_ASSERT(colorNode);
  colorNode->GetLookupTable()->SetRange(min, max);
}

void qSlicerColorsModuleWidget::copyCurrentColorNode()
{
  Q_D(qSlicerColorsModuleWidget);
  vtkMRMLColorNode* currentNode = vtkMRMLColorNode::SafeDownCast(
    d->ColorTableComboBox->currentNode());
  QString newColorName = QInputDialog::getText(
    this, "Transfer function name",
    "Please select a new name for the transfer function to create from copy",
    QLineEdit::Normal,
    QString(currentNode->GetName()) + QString("Copy"));
  if (newColorName.isEmpty())
    {
    return;
    }
  vtkMRMLColorTableNode *colorNode = vtkMRMLColorTableNode::New();
  colorNode->SetName(newColorName.toLatin1());
  colorNode->SetTypeToUser();
  colorNode->SetAttribute("Category", "User Generated");
  if (currentNode->GetLookupTable())
    {
    double* range = colorNode->GetLookupTable()->GetRange();
    colorNode->GetLookupTable()->SetRange(range[0], range[1]);
    }
  colorNode->SetNumberOfColors(currentNode->GetNumberOfColors());
  for (int i = 0; i < currentNode->GetNumberOfColors(); ++i)
    {
    double color[4];
    currentNode->GetColor(i, color);
    colorNode->SetColor(i, currentNode->GetColorName(i), color[0], color[1], color[2]);
    }
  this->mrmlScene()->AddNode(colorNode);
  colorNode->Delete();
  d->ColorTableComboBox->setCurrentNode(colorNode);
}
