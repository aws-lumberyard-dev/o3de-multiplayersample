/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <Multiplayer/Session/SessionNotifications.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace MultiplayerSample
{
    class MultiplayerSampleServerSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public Multiplayer::SessionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(MultiplayerSampleServerSystemComponent, "{5768429C-65CE-47B2-854F-3D1BECEB9D0C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    
        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        ////////////////////////////////////////////////////////////////////////

        // Multiplayer::SessionNotificationBus::Handler overrides
        bool OnSessionHealthCheck() override;
        bool OnCreateSessionBegin(const Multiplayer::SessionConfig& sessionConfig) override;
        void OnCreateSessionEnd() override;
        bool OnDestroySessionBegin() override;
        void OnDestroySessionEnd() override {}
        void OnUpdateSessionBegin(const Multiplayer::SessionConfig& sessionConfig, const AZStd::string& updateReason) override;
        void OnUpdateSessionEnd() override {}
        ////////////////////////////////////////////////////////////////////////

        void StartBackfillProcess();
        void StopBackfillProcess();
        void BackfillProcess();

    private:
        AZStd::string m_matchmakingTicketId;
        AZStd::thread m_backfillThread;
        AZStd::binary_semaphore m_waitEvent;
        bool m_isBackfilled = false;
    };
}
