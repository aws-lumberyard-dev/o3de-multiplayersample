/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerSampleAWSGameLiftServerSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Request/AWSGameLiftServerRequestBus.h>

namespace MultiplayerSample
{
    void MultiplayerSampleAWSGameLiftServerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MultiplayerSampleAWSGameLiftServerSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerSampleAWSGameLiftServerService"));
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerSampleAWSGameLiftServerService"));
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));
        required.push_back(AZ_CRC_CE("MultiplayerService"));
        required.push_back(AZ_CRC_CE("AWSGameLiftServerService"));
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::Init()
    {
        ;
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::Activate()
    {
        AZ_Info("MultiplayerSampleAWSGameLiftServerSystemComponent", "Activated!\n");

        Multiplayer::SessionNotificationBus::Handler::BusConnect();

        AWSGameLift::AWSGameLiftServerRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftServerRequestBus::Events::NotifyGameLiftProcessReady);
    }

    void MultiplayerSampleAWSGameLiftServerSystemComponent::Deactivate()
    {
        Multiplayer::SessionNotificationBus::Handler::BusDisconnect();
    }

    bool MultiplayerSampleAWSGameLiftServerSystemComponent::OnSessionHealthCheck()
    {
        // TODO: any game stats or conditions we should check?
        return true;
    }

    bool MultiplayerSampleAWSGameLiftServerSystemComponent::OnCreateSessionBegin(const Multiplayer::SessionConfig& sessionConfig)
    {
        // TODO: any game state we should update?
        AZ_UNUSED(sessionConfig);
        return true;
    }
}