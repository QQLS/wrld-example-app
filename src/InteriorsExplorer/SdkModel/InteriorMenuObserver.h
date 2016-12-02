// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

#pragma once

#include "Types.h"
#include "InteriorSelectionModel.h"
#include "InteriorMetaDataRepository.h"
#include "TagSearchRepository.h"

namespace ExampleApp
{
    namespace InteriorsExplorer
    {
        namespace SdkModel
        {
            class InteriorMenuObserver : private Eegeo::NonCopyable
            {
            public:
                InteriorMenuObserver(Eegeo::Resources::Interiors::InteriorSelectionModel& interiorSelectionModel,
                                     Eegeo::Resources::Interiors::MetaData::IInteriorMetaDataRepository& interiorMetaDataRepo,
                                     TagSearch::View::ITagSearchRepository& tagSearchRepository);
                ~InteriorMenuObserver();
                TagSearch::View::ITagSearchRepository& GetTagsRepository() { return m_tagSearchRepository; }
                
            private:
                enum TransitionState
                {
                    EnteringBuilding = 0,
                    LeavingBuilding,
                    SwitchingBuilding,
                    NoTransition
                };
                
                void OnSelectionChanged(const Eegeo::Resources::Interiors::InteriorId& interiorId);
                
                Eegeo::Helpers::TCallback1<InteriorMenuObserver, const Eegeo::Resources::Interiors::InteriorId> m_interiorSelectionChangedCallback;
                void OnEnterInterior(const Eegeo::Resources::Interiors::InteriorId& interiorId, const TransitionState& transitionState);
                void OnExitInterior();
                void ClearTagSearchModelTracker();
                
                TagSearch::View::ITagSearchRepository& m_tagSearchRepository;
                Eegeo::Resources::Interiors::InteriorSelectionModel& m_interiorSelectionModel;
                Eegeo::Resources::Interiors::MetaData::IInteriorMetaDataRepository& m_interiorMetaDataRepo;
                TagSearch::View::TagSearchRepository m_previousTagSearchRepository;
                TransitionState HandleTransitionStates();
                std::vector<const TagSearch::View::TagSearchModel*> m_tagSearchModelTracker;
                
                bool m_hasSelectedInterior;
                bool m_defaultTagsSaved;
            };

        }
    }
}