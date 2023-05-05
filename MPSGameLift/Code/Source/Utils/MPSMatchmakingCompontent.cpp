/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Utils/MPSMatchmakingComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#pragma optimize("",off)
namespace MPSGameLift
{

    void MPSMatchmakingComponent::Activate()
    {
        MPSMatchmakingComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MPSMatchmakingComponent::Deactivate()
    {
        MPSMatchmakingComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void MPSMatchmakingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MPSMatchmakingComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MPSMatchmakingComponent>("MPSLatencyComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                ->DataElement(AZ::Edit::UIHandlers::Default, &MPSLatencyComponent::m_regions_, "AWS Regions", "List of AWS Regions")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MPSMatchmakingComponent>("MPSMatchmakingComponent Group")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ;
        
            behaviorContext->EBus<MPSLatencyComponentRequestBus>("MPSMatchmakingComponentRequests")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Event("RequestLatencies", &MPSLatencyComponentRequestBus::Events::RequestLatencies)
                ->Event("HasLatencies", &MPSLatencyComponentRequestBus::Events::HasLatencies)
                ->Event("GetLatencyForRegion", &MPSLatencyComponentRequestBus::Events::GetLatencyForRegion)
                ;
        }
    }

    void MPSMatchmakingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MPSMatchmakingService"));
    }

    void MPSMatchmakingComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MPSMatchmakingService"));
    }

    void MPSMatchmakingComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MPSMatchmakingComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }




}
#pragma optimize("",on)