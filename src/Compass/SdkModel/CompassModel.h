// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include <map>
#include "Types.h"
#include "ICompassModel.h"
#include "Rendering.h"
#include "CallbackCollection.h"
#include "AppCamera.h"
#include "Location.h"
#include "IMetricsService.h"
#include "AppModes.h"
#include "AlertBox.h"
#include "ISingleOptionAlertBoxDismissedHandler.h"
#include "InteriorsNavigation.h"
#include "InteriorsExplorer.h"
#include "SenionLocation.h"

namespace ExampleApp
{
    namespace Compass
    {
        namespace SdkModel
        {
            class CompassModel : public ICompassModel, private Eegeo::NonCopyable
            {
                Eegeo::Location::NavigationService& m_navigationService;
                InteriorsNavigation::SdkModel::IInteriorsNavigationService& m_interiorsNavigationService;
                ExampleApp::SenionLocation::SdkModel::ISenionLocationService& m_locationService;
                ExampleApp::AppCamera::SdkModel::IAppCameraController& m_cameraController;
                Eegeo::Helpers::CallbackCollection0 m_compassAllowedChangedCallbacks;
                Eegeo::Helpers::CallbackCollection0 m_gpsModeChangedCallbacks;
                Eegeo::Helpers::CallbackCollection0 m_gpsModeUnauthorizedCallbacks;
                GpsMode::Values m_gpsMode;
                bool m_exitInteriorTriggered;
                std::map<Eegeo::Location::NavigationService::GpsMode, GpsMode::Values> m_compassGpsModeToNavigationGpsMode;
                std::map<GpsMode::Values, Eegeo::Location::NavigationService::GpsMode> m_navigationGpsModeToCompassGpsMode;
                std::map<GpsMode::Values, const char*> m_gpsModeToString;
                
                Metrics::IMetricsService& m_metricsService;

                AppModes::SdkModel::IAppModeModel& m_appModeModel;
                InteriorsExplorer::SdkModel::InteriorsExplorerModel& m_interiorExplorerModel;
                Eegeo::Helpers::TCallback0<CompassModel> m_appModeChangedCallback;
                
                Eegeo::UI::NativeAlerts::IAlertBoxFactory& m_alertBoxFactory;
                Eegeo::UI::NativeAlerts::TSingleOptionAlertBoxDismissedHandler<CompassModel> m_failAlertHandler;
                
            public:

                CompassModel(Eegeo::Location::NavigationService& navigationService,
                             InteriorsNavigation::SdkModel::IInteriorsNavigationService& interiorsNavigationService,
                             ExampleApp::SenionLocation::SdkModel::ISenionLocationService& locationService,
                             ExampleApp::AppCamera::SdkModel::IAppCameraController& Cameracontroller,
                             Metrics::IMetricsService& metricsService,
                             InteriorsExplorer::SdkModel::InteriorsExplorerModel& interiorExplorerModel,
                             AppModes::SdkModel::IAppModeModel& appModeModel,
                             Eegeo::UI::NativeAlerts::IAlertBoxFactory& alertBoxFactory);

                ~CompassModel();

                bool GetGpsModeActive() const;

                GpsMode::Values GetGpsMode() const;

                void DisableGpsMode();

                void TryUpdateToNavigationServiceGpsMode(Eegeo::Location::NavigationService::GpsMode value);

                float GetHeadingRadians() const;

                float GetHeadingDegrees() const;

                void CycleToNextGpsMode();

                void InsertGpsModeChangedCallback(Eegeo::Helpers::ICallback0& callback);

                void RemoveGpsModeChangedCallback(Eegeo::Helpers::ICallback0& callback);
                
                void InsertGpsModeUnauthorizedCallback(Eegeo::Helpers::ICallback0& callback);
                
                void RemoveGpsModeUnauthorizedCallback(Eegeo::Helpers::ICallback0& callback);

            private:
                void SetGpsMode(GpsMode::Values value);
                
                void OnAppModeChanged();
                
                void OnFailedToGetLocation();
                
                bool NeedsToExitInterior(GpsMode::Values gpsMode);
            };
        }
    }
}
