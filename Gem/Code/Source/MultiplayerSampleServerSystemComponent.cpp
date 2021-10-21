/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerSampleServerSystemComponent.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzNetworking/Framework/INetworking.h>

#include <Request/AWSGameLiftServerRequestBus.h>

namespace MultiplayerSample
{
    using namespace AzNetworking;

    AZ_CVAR(bool, sv_manualBackfill, false, nullptr, AZ::ConsoleFunctorFlags::Null, "");

    void MultiplayerSampleServerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MultiplayerSampleServerSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MultiplayerSampleServerSystemComponent>("MultiplayerSampleServer", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MultiplayerSampleServerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerSampleServerService"));
    }

    void MultiplayerSampleServerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerSampleServerService"));
    }

    void MultiplayerSampleServerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));
        required.push_back(AZ_CRC_CE("MultiplayerService"));
        required.push_back(AZ_CRC_CE("AWSGameLiftServerService"));
    }

    void MultiplayerSampleServerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void MultiplayerSampleServerSystemComponent::Init()
    {
        ;
    }

    void MultiplayerSampleServerSystemComponent::Activate()
    {
        Multiplayer::SessionNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        AWSGameLift::AWSGameLiftServerRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftServerRequestBus::Events::NotifyGameLiftProcessReady);
    }

    void MultiplayerSampleServerSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        Multiplayer::SessionNotificationBus::Handler::BusDisconnect();
    }

    void MultiplayerSampleServerSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

    int MultiplayerSampleServerSystemComponent::GetTickOrder()
    {
        // Tick immediately after the multiplayer system component
        return AZ::TICK_PLACEMENT + 2;
    }

    bool MultiplayerSampleServerSystemComponent::OnSessionHealthCheck()
    {
        return true;
    }

    bool MultiplayerSampleServerSystemComponent::OnCreateSessionBegin(const Multiplayer::SessionConfig& sessionConfig)
    {
        AZ_UNUSED(sessionConfig);
        return true;
    }

    void MultiplayerSampleServerSystemComponent::OnCreateSessionEnd()
    {
        m_isBackfilled = false;
        StartBackfillProcess();
    }

    bool MultiplayerSampleServerSystemComponent::OnDestroySessionBegin()
    {
        StopBackfillProcess();
        return true;
    }

    void MultiplayerSampleServerSystemComponent::OnUpdateSessionBegin(const Multiplayer::SessionConfig& sessionConfig, const AZStd::string& updateReason)
    {
        AZ_UNUSED(sessionConfig);
        if (updateReason == "MATCHMAKING_DATA_UPDATED")
        {
            m_isBackfilled = true;
            StopBackfillProcess();
        }
        else if (updateReason == "BACKFILL_FAILED" || updateReason == "BACKFILL_TIMED_OUT" || updateReason == "BACKFILL_CANCELLED")
        {
            StartBackfillProcess();
        }
    }

    void MultiplayerSampleServerSystemComponent::StartBackfillProcess()
    {
        m_waitEvent.release();
        if (m_backfillThread.joinable())
        {
            m_backfillThread.join();
        }
        m_waitEvent.acquire();
        if (sv_manualBackfill && !m_isBackfilled)
        {
            AZ_TracePrintf("MultiplayerSample", "Start multiplayer sample server manual backfill");
            m_backfillThread = AZStd::thread(AZStd::bind(&MultiplayerSampleServerSystemComponent::BackfillProcess, this));
        }
    }

    void MultiplayerSampleServerSystemComponent::StopBackfillProcess()
    {
        m_waitEvent.release();
        if (m_backfillThread.joinable())
        {
            m_backfillThread.join();
        }
    }

    void MultiplayerSampleServerSystemComponent::BackfillProcess()
    {
        m_matchmakingTicketId.clear();
        m_waitEvent.try_acquire_for(AZStd::chrono::seconds(60));

        m_matchmakingTicketId = AZ::Uuid::CreateRandom().ToString<AZStd::string>(false, false);
        AZStd::vector<AWSGameLift::AWSGameLiftPlayer> players;
        AZ_TracePrintf("MultiplayerSample", "Initiate multiplayer sample server manual backfill");
        AWSGameLift::AWSGameLiftServerRequestBus::Broadcast(
            &AWSGameLift::AWSGameLiftServerRequestBus::Events::StartMatchBackfill, m_matchmakingTicketId, players);
    }
}

