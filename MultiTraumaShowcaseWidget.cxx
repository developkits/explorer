#include "MultiTraumaShowcaseWidget.h"
#include "ui_MultiTraumaShowcase.h"

#include <QMutex>

#include "cdm/CommonDataModel.h"
#include "PulsePhysiologyEngine.h"
#include "cdm/scenario/SEDataRequestManager.h"
#include "cdm/properties/SEScalarTime.h"
#include "cdm/properties/SEScalar0To1.h"
#include "cdm/properties/SEScalarVolume.h"
#include "cdm/properties/SEScalarMassPerVolume.h"
#include "cdm/properties/SEScalarVolumePerTime.h"
#include "cdm/substance/SESubstance.h"
#include "cdm/substance/SESubstanceManager.h"
#include "cdm/patient/actions/SESubstanceBolus.h"
#include "cdm/patient/actions/SESubstanceCompoundInfusion.h"
#include "cdm/patient/actions/SEHemorrhage.h"
#include "cdm/patient/actions/SETensionPneumothorax.h"
#include "cdm/patient/actions/SENeedleDecompression.h"

class MultiTraumaShowcaseWidget::Controls : public Ui::MultiTraumaShowcaseWidget
{
public:
  Controls(QPulse& qp) : Pulse(qp) {}
  QPulse&              Pulse;
  double               HemorrhageRate_mL_Per_min=0;
  double               PneumothoraxSeverity = 0;
  cdm::eSide           PneumothoraxSide;
  bool                 ApplyHemorrhage = false;
  bool                 ApplyPneumothorax = false;
  bool                 ApplyPressure = false;
  bool                 ApplyNeedleDecompression = false;
  bool                 ApplyTourniquet = false;
  bool                 InfuseSaline = false;
  bool                 InjectMorphine = false;
  QMutex               Mutex;
};

MultiTraumaShowcaseWidget::MultiTraumaShowcaseWidget(QPulse& qp, QWidget *parent, Qt::WindowFlags flags) : QDockWidget(parent,flags)
{
  m_Controls = new Controls(qp);
  m_Controls->setupUi(this);

  m_Controls->FlowRateEdit->setValidator(new QDoubleValidator(0, 500, 1, this));

  connect(m_Controls->ApplyHemorrhageButton, SIGNAL(clicked()), this, SLOT(ApplyHemorrhage()));
  connect(m_Controls->ApplyPneumoButton, SIGNAL(clicked()), this, SLOT(ApplyPneumothorax()));
  connect(m_Controls->ApplyPressureButton, SIGNAL(clicked()), this, SLOT(ApplyPressure()));
  connect(m_Controls->NeedleDecompressButton, SIGNAL(clicked()), this, SLOT(ApplyNeedleDecompression()));
  connect(m_Controls->ApplyTournyButton, SIGNAL(clicked()), this, SLOT(ApplyTourniquet()));
  connect(m_Controls->InfuseSalineButton, SIGNAL(clicked()), this, SLOT(InfuseSaline()));
  connect(m_Controls->InjectMorphineButton, SIGNAL(clicked()), this, SLOT(InjectMorphine()));
}

MultiTraumaShowcaseWidget::~MultiTraumaShowcaseWidget()
{
  delete m_Controls;
}

void MultiTraumaShowcaseWidget::ConfigurePulse(PhysiologyEngine& pulse, SEDataRequestManager& drMgr)
{
  m_Controls->HemorrhageRate_mL_Per_min = 0;
  m_Controls->PneumothoraxSeverity = 0;
  m_Controls->PneumothoraxSide;
  m_Controls->ApplyHemorrhage = false;
  m_Controls->ApplyPneumothorax = false;
  m_Controls->ApplyPressure = false;
  m_Controls->ApplyNeedleDecompression = false;
  m_Controls->ApplyTourniquet = false;
  m_Controls->InfuseSaline = false;
  m_Controls->InjectMorphine = false;

  m_Controls->ApplyHemorrhageButton->setEnabled(true);
  m_Controls->FlowRateEdit->setEnabled(true);
  m_Controls->ApplyPneumoButton->setEnabled(true);
  m_Controls->SeveritySlider->setEnabled(true);
  m_Controls->PneumothoraxTypeCombo->setEnabled(true);
  m_Controls->ApplyPressureButton->setEnabled(false);
  m_Controls->NeedleDecompressButton->setEnabled(false);
  m_Controls->ApplyTournyButton->setEnabled(false);
  m_Controls->InfuseSalineButton->setEnabled(false);
  m_Controls->InjectMorphineButton->setEnabled(false);

  if(!pulse.LoadStateFile("states/Soldier@0s.pba"))
    throw CommonDataModelException("Unable to load state file");
  m_Controls->Pulse.GetLogBox().append("Combining the tension pneumothorax with the blood loss from the hemorrhage pushes and eventually exceeds the limits of the homeostatic control mechanisms.");
  m_Controls->Pulse.ScrollLogBox();
  // Fill out any data requsts that we want to have plotted
  drMgr.CreatePhysiologyDataRequest("BloodVolume", VolumeUnit::L);
  drMgr.CreatePhysiologyDataRequest("TidalVolume", VolumeUnit::mL);
  drMgr.CreatePhysiologyDataRequest("CardiacOutput", VolumePerTimeUnit::L_Per_min);
  drMgr.CreateGasCompartmentDataRequest(pulse::PulmonaryCompartment::LeftLung, "Volume", VolumeUnit::mL);
  drMgr.CreateGasCompartmentDataRequest(pulse::PulmonaryCompartment::RightLung, "Volume", VolumeUnit::mL);
  const SESubstance* Morphine = pulse.GetSubstanceManager().GetSubstance("Morphine");
  drMgr.CreateSubstanceDataRequest(*Morphine, "PlasmaConcentration", MassPerVolumeUnit::ug_Per_mL);
}

void MultiTraumaShowcaseWidget::ProcessPhysiology(PhysiologyEngine& pulse)
{
  // This is where we pull data from pulse, and push any actions to it
  m_Controls->Mutex.lock();
  if (m_Controls->ApplyPneumothorax)
  {
    m_Controls->ApplyPneumothorax = false;
    SETensionPneumothorax pneumothorax;
    pneumothorax.SetSide(m_Controls->PneumothoraxSide);
    pneumothorax.SetType(cdm::eGate::Closed);
    pneumothorax.GetSeverity().SetValue(m_Controls->PneumothoraxSeverity);
    pulse.ProcessAction(pneumothorax);
  }
  if (m_Controls->ApplyNeedleDecompression)
  {
    m_Controls->ApplyNeedleDecompression = false;
    SENeedleDecompression needle;
    needle.SetActive(true);
    needle.SetSide(m_Controls->PneumothoraxSide);
    pulse.ProcessAction(needle);
  }

  if (m_Controls->ApplyHemorrhage)
  {
    m_Controls->ApplyHemorrhage = false;
    SEHemorrhage hemorrhage;
    hemorrhage.GetRate().SetValue(m_Controls->HemorrhageRate_mL_Per_min, VolumePerTimeUnit::mL_Per_min);
    hemorrhage.SetCompartment(pulse::VascularCompartment::RightLeg);
    pulse.ProcessAction(hemorrhage);
  }
  if (m_Controls->ApplyPressure)
  {
    m_Controls->ApplyPressure = false;
    SEHemorrhage hemorrhage;
    hemorrhage.GetRate().SetValue(m_Controls->HemorrhageRate_mL_Per_min*0.15, VolumePerTimeUnit::mL_Per_min);
    hemorrhage.SetCompartment(pulse::VascularCompartment::RightLeg);
    pulse.ProcessAction(hemorrhage);
  }
  if (m_Controls->ApplyTourniquet)
  {
    m_Controls->ApplyTourniquet = false;
    SEHemorrhage hemorrhage;
    hemorrhage.GetRate().SetValue(0,VolumePerTimeUnit::mL_Per_min);
    hemorrhage.SetCompartment(pulse::VascularCompartment::RightLeg);
    pulse.ProcessAction(hemorrhage);
  }

  if (m_Controls->InfuseSaline)
  {
    m_Controls->InfuseSaline = false;
    const SESubstanceCompound* Saline = pulse.GetSubstanceManager().GetCompound("Saline");
    SESubstanceCompoundInfusion   SalineInfusion(*Saline);
    SalineInfusion.GetBagVolume().SetValue(500, VolumeUnit::mL);
    SalineInfusion.GetRate().SetValue(100, VolumePerTimeUnit::mL_Per_min);
    pulse.ProcessAction(SalineInfusion);
  }

  if (m_Controls->InjectMorphine)
  {
    m_Controls->InjectMorphine = false;
    const SESubstance* Morphine = pulse.GetSubstanceManager().GetSubstance("Morphine");
    SESubstanceBolus   MorphineBolus(*Morphine);
    MorphineBolus.GetConcentration().SetValue(1000, MassPerVolumeUnit::ug_Per_mL);
    MorphineBolus.GetDose().SetValue(5.0, VolumeUnit::mL);
    MorphineBolus.SetAdminRoute(cdm::SubstanceBolusData_eAdministrationRoute_Intravenous);
    pulse.ProcessAction(MorphineBolus);
  }
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::ApplyHemorrhage()
{
  m_Controls->Mutex.lock();
  m_Controls->HemorrhageRate_mL_Per_min = m_Controls->FlowRateEdit->text().toDouble();
  m_Controls->ApplyHemorrhage = true;
  m_Controls->ApplyHemorrhageButton->setDisabled(true);
  m_Controls->FlowRateEdit->setDisabled(true);
  m_Controls->ApplyPressureButton->setEnabled(true);
  m_Controls->Pulse.GetLogBox().append("Applying hemorrhage");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::ApplyPneumothorax()
{
  m_Controls->Mutex.lock();
  m_Controls->PneumothoraxSide = m_Controls->PneumothoraxTypeCombo->currentIndex()==0?cdm::eSide::Left:cdm::eSide::Right;
  m_Controls->PneumothoraxSeverity = m_Controls->SeveritySlider->value();
  m_Controls->ApplyPneumothorax = true;
  m_Controls->ApplyPneumoButton->setDisabled(true);
  m_Controls->SeveritySlider->setDisabled(true);
  m_Controls->PneumothoraxTypeCombo->setDisabled(true);
  m_Controls->NeedleDecompressButton->setEnabled(true);
  m_Controls->Pulse.GetLogBox().append("Applying Pneumothorax");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::ApplyPressure()
{
  m_Controls->Mutex.lock();
  m_Controls->ApplyPressure = true;
  m_Controls->ApplyPressureButton->setDisabled(true);
  m_Controls->ApplyTournyButton->setEnabled(true);
  m_Controls->Pulse.GetLogBox().append("Applying pressure to the wound");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::ApplyNeedleDecompression()
{
  m_Controls->Mutex.lock();
  m_Controls->ApplyNeedleDecompression = true;
  m_Controls->NeedleDecompressButton->setEnabled(false);
  m_Controls->Pulse.GetLogBox().append("Applying Needle Decompression");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::ApplyTourniquet()
{
  m_Controls->Mutex.lock();
  m_Controls->ApplyTourniquet = true;
  m_Controls->ApplyTournyButton->setEnabled(false);
  m_Controls->InfuseSalineButton->setEnabled(true);
  m_Controls->InjectMorphineButton->setEnabled(true);
  m_Controls->Pulse.GetLogBox().append("Applying Tourniquet");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::InfuseSaline()
{
  m_Controls->Mutex.lock();
  m_Controls->InfuseSaline = true;
  m_Controls->InfuseSalineButton->setEnabled(false);
  m_Controls->Pulse.GetLogBox().append("Infusing saline");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}

void MultiTraumaShowcaseWidget::InjectMorphine()
{
  m_Controls->Mutex.lock();
  m_Controls->InjectMorphine = true;
  m_Controls->InjectMorphineButton->setEnabled(false);
  m_Controls->Pulse.GetLogBox().append("Injecting a bolus of morphine");
  m_Controls->Pulse.ScrollLogBox();
  m_Controls->Mutex.unlock();
}
