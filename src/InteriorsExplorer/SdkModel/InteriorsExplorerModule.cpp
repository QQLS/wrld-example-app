// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#include "InteriorsExplorerModule.h"
#include "InteriorsExplorerViewModel.h"
#include "InteriorsExplorerModel.h"
#include "InteriorWorldPinController.h"
#include "GlobeCameraController.h"
#include "InteriorsCameraControllerFactory.h"
#include "InteriorsCameraController.h"
#include "GlobeCameraTouchController.h"
#include "InteriorVisibilityUpdater.h"
#include "InteriorExplorerUserInteractionModel.h"
#include "IInitialExperienceModel.h"
#include "InteriorsExplorerFloorDraggedObserver.h"
#include "InteriorsUINotificationService.h"
#include "InteriorCameraController.h"
#include "InteriorTransitionModel.h"
#include "InteriorSelectionModel.h"
#include "InteriorInteractionModel.h"
#include "InteriorMarkerModelRepository.h"
#include "IWorldPinsService.h"
#include "IVisualMapService.h"
#include "IMetricsService.h"
#include "IWorldPinIconMapping.h"

namespace ExampleApp
{
    namespace InteriorsExplorer
    {
        namespace SdkModel
        {
            void InteriorsExplorerModule::Register(const TContainerBuilder& builder)
            {
                builder->registerType<InteriorExplorerUserInteractionModel>().singleInstance();
                builder->registerType<InteriorVisibilityUpdater>().singleInstance();
                builder->registerInstanceFactory([](Hypodermic::ComponentContext& context)
                                                   {
                                                       auto factory = context.resolve<Eegeo::Resources::Interiors::InteriorsCameraControllerFactory>();
                                                       auto touchController = std::shared_ptr<Eegeo::Camera::GlobeCamera::GlobeCameraTouchController>(factory->CreateTouchController());
                                                       auto cameraController = std::shared_ptr<Eegeo::Camera::GlobeCamera::GlobeCameraController>(factory->CreateInteriorGlobeCameraController(*touchController));
                                                       auto sdkCameraController = std::shared_ptr<Eegeo::Resources::Interiors::InteriorsCameraController>(factory->CreateInteriorsCameraController(*touchController, *cameraController));
                                                       return std::make_shared<InteriorCameraController>(touchController, cameraController, sdkCameraController);
                                                   }).as<IInteriorCameraController>().singleInstance();
                builder->registerType<InteriorWorldPinController>().singleInstance();
                builder->registerType<InteriorsExplorerModel>().singleInstance();
                builder->registerInstanceFactory([](Hypodermic::ComponentContext& context)
                                                   {
                                                       return std::make_shared<View::InteriorsExplorerViewModel>(false, context.resolve<Eegeo::Helpers::IIdentityProvider>(), context.resolve<ExampleAppMessaging::TMessageBus>());
                                                   }).singleInstance();
                builder->registerType<InteriorsExplorerFloorDraggedObserver>().singleInstance();
                builder->registerType<InteriorsUINotificationService>().singleInstance();
            }
            
            void InteriorsExplorerModule::RegisterLeaves()
            {
                RegisterLeaf<InteriorsExplorerFloorDraggedObserver>();
                RegisterLeaf<InteriorsUINotificationService>();
            }
        }
    }
}
