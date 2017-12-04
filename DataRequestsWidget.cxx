#include "DataRequestsWidget.h"

#include <QVBoxLayout>
#include <QMutex>

#include "cdm/utils/FileUtils.h"

#include "cdm/system/equipment/electrocardiogram/SEElectroCardioGramWaveformInterpolator.h"
#include "cdm/properties/SEFunctionElectricPotentialVsTime.h"

class DataRequestsWidget::Controls
{
public:
  Controls(QTextEdit& log) : LogBox(log) {}
  QTextEdit&                         LogBox;
  QVBoxLayout*                       Layout;
  QMutex                             Mutex;
  std::vector<std::string>           Graphs; // Instead of plots, just names for now
  std::vector<double>                Values; // New value for the plot
  int                                Count = 0;//Just outputting data to the log every 5 seconds, take out when plots are working
};

DataRequestsWidget::DataRequestsWidget(QTextEdit& log, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent,flags)
{
  m_Controls = new Controls(log);

  connect(this, SIGNAL(UIChanged()), this, SLOT(UpdateUI()));
  connect(this, SIGNAL(PulseChanged()), this, SLOT(PulseUpdate()));
}

DataRequestsWidget::~DataRequestsWidget()
{
  delete m_Controls;
}

void DataRequestsWidget::Reset()
{
  m_Controls->Count = 0;
}

void DataRequestsWidget::BuildGraphs(PhysiologyEngine& pulse)
{
  std::stringstream ss;
  SEDataRequestManager& drMgr = pulse.GetEngineTracker()->GetDataRequestManager();
  std::string title;
  for (SEDataRequest* dr : drMgr.GetDataRequests())
  {
    
    switch (dr->GetCategory())
    {
    case cdm::DataRequestData_eCategory_Patient:
      title = "Patient " + dr->GetPropertyName() + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_Physiology:
      title = dr->GetPropertyName() + "(" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_Environment:
      title = dr->GetPropertyName() + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_GasCompartment:
    case cdm::DataRequestData_eCategory_LiquidCompartment:
      if (dr->HasSubstanceName())
        title = dr->GetCompartmentName() + " " + dr->GetSubstanceName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      else
        title = dr->GetCompartmentName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_ThermalCompartment:
      title = dr->GetCompartmentName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_TissueCompartment:
      title = dr->GetCompartmentName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_Substance:
      if (dr->HasCompartmentName())
        title = dr->GetSubstanceName() + " " + dr->GetCompartmentName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      else
        title = dr->GetSubstanceName() + " " + dr->GetPropertyName() + " " + " (" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_AnesthesiaMachine:
      title = dr->GetPropertyName() + "(" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_ECG:
      title = dr->GetPropertyName() + "(" + dr->GetUnit()->GetString() + ")";
      break;
    case cdm::DataRequestData_eCategory_Inhaler:
      title = dr->GetPropertyName() + "(" + dr->GetUnit()->GetString() + ")";
      break;
    }
    if (!pulse.GetEngineTracker()->TrackRequest(*dr))
    {// Could not hook this up, get rid of it
      ss << "Unable to find data for " << title;
      m_Controls->LogBox.append(ss.str().c_str());
      continue;
    }
    m_Controls->Graphs.push_back(title);
    m_Controls->Values.push_back(0);
  }
  
}

void DataRequestsWidget::ProcessPhysiology(PhysiologyEngine& pulse)
{
  // This is where we pull data from pulse, and push any actions to it
  size_t i = 0;
  pulse.GetEngineTracker()->PullData();
  for (SEDataRequest* dr : pulse.GetEngineTracker()->GetDataRequestManager().GetDataRequests())
  {
    m_Controls->Values[i++] = 0;//I need to check this into plulse-> pulse.GetEngineTracker()->GetScalar(*dr)->GetValue();
  }
  emit PulseChanged(); // Call this if you need to update the UI with data from pulse
}

void DataRequestsWidget::UpdateUI()
{

}

void DataRequestsWidget::PulseUpdate()
{
  // This is where we take the pulse data we pulled and push it to a UI widget
  if (++m_Controls->Count == 50)
  {
    m_Controls->Count = 0;
    m_Controls->Mutex.lock();
    for (size_t i = 0; i < m_Controls->Graphs.size(); i++)
      m_Controls->LogBox.append(QString("%1 : %2").arg(m_Controls->Graphs[i].c_str()).arg(m_Controls->Values[i]));
    m_Controls->Mutex.unlock();
  }
}
