// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "InteriorsExplorer.h"
#include "BidirectionalBus.h"
#include "Menu.h"
#include "MyPinCreation.h"
#include "ScreenControlViewModelIncludes.h"
#include "InteriorsExplorerStateChangedMessage.h"
#include "InteriorsExplorerFloorSelectedMessage.h"
#include "IAppModeModel.h" // FM: Included for definition of AppMode, may want to extract AppMode to separate .h
#include <sstream>
#include "InteriorsExplorerUINotifyMessage.h"

namespace ExampleApp
{
    namespace InteriorsExplorer
    {
        namespace View
        {
            class InteriorsExplorerController : private Eegeo::NonCopyable
            {
            public:
                
                InteriorsExplorerController(const std::shared_ptr<SdkModel::InteriorsExplorerModel>& model,
                                            const std::shared_ptr<IInteriorsExplorerView>& view,
                                            const std::shared_ptr<InteriorsExplorerViewModel>& viewModel,
                                            const std::shared_ptr<ExampleAppMessaging::TMessageBus>& messageBus);
                
                ~InteriorsExplorerController();
                
            private:
                
                void OnDismiss();
                void OnSelectFloor(int& selected);
                void OnFloorSelectionDragged(float& dragParam);
                void OnFloorSelected(const InteriorsExplorerFloorSelectedMessage& message);
                void OnStateChanged(const InteriorsExplorerStateChangedMessage& message);
                void OnViewStateChangeScreenControl(ScreenControl::View::IScreenControlViewModel &viewModel, float &state);
                void OnAppModeChanged(const AppModes::AppModeChangedMessage& message);
                void OnInteriorsUINotificationRequired(const InteriorsExplorerUINotifyMessage& message);
                
                const std::shared_ptr<SdkModel::InteriorsExplorerModel> m_model;
                const std::shared_ptr<IInteriorsExplorerView> m_view;
                const std::shared_ptr<InteriorsExplorerViewModel> m_viewModel;
                
                Eegeo::Helpers::TCallback0<InteriorsExplorerController> m_dismissedCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, int> m_selectFloorCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, float> m_draggingFloorSelectionCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, const InteriorsExplorerStateChangedMessage&> m_stateChangedCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, const InteriorsExplorerFloorSelectedMessage&> m_floorSelectedCallback;
                Eegeo::Helpers::TCallback2<InteriorsExplorerController, ScreenControl::View::IScreenControlViewModel&, float> m_viewStateCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, const AppModes::AppModeChangedMessage&> m_appModeChangedCallback;
                Eegeo::Helpers::TCallback1<InteriorsExplorerController, const InteriorsExplorerUINotifyMessage&> m_interiorsUINotificationCallback;
                
                AppModes::SdkModel::AppMode m_appMode;
                
                const std::shared_ptr<ExampleAppMessaging::TMessageBus> m_messageBus;
            };
        }
    }
}
