// Copyright eeGeo Ltd (2012-2016), All Rights Reserved

#pragma once

#include "SenionLabLocationController.h"
#include "SenionLabLocationManagerInterop.h"
#include "SenionLabLocationService.h"
#include "iOSAlertBoxFactory.h"
#include "ICallback.h"
#include "AppModeModel.h"
#include "InteriorsExplorer.h"
#include "ApplicationConfiguration.h"
#include "ILocationService.h"

@class SenionLabLocationManager;

namespace ExampleApp
{
    namespace SenionLab
    {
        class SenionLabLocationModule
        {
        public:
            SenionLabLocationModule(Eegeo::Resources::Interiors::InteriorInteractionModel& interiorInteractionModel,
                                    const Eegeo::Rendering::EnvironmentFlatteningService& environmentFlatteningService,
                                    const ExampleApp::ApplicationConfig::ApplicationConfiguration& applicationConfiguration,
                                    Eegeo::Location::ILocationService& defaultLocationService,
                                    Eegeo::UI::NativeAlerts::iOS::iOSAlertBoxFactory& iOSAlertBoxFactory,
									ExampleAppMessaging::TMessageBus& messageBus);
            ~SenionLabLocationModule();
            
            InteriorsPosition::SdkModel::SenionLab::SenionLabLocationService& GetLocationService() { return m_locationService; }
            InteriorsPosition::SdkModel::SenionLab::SenionLabLocationController& GetLocationController() { return m_locationController; }
            
        private:
            InteriorsPosition::SdkModel::SenionLab::SenionLabLocationService m_locationService;
            SenionLabLocationManagerInterop m_locationManager;
            InteriorsPosition::SdkModel::SenionLab::SenionLabLocationController m_locationController;
        };
    }
}
