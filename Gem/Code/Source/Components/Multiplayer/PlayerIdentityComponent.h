/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MultiplayerSampleTypes.h>
#include <PlayerIdentityBus.h>
#include <Source/AutoGen/PlayerIdentityComponent.AutoComponent.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Font/FontInterface.h>

#if AZ_TRAIT_CLIENT
    namespace AZ::RPI
    {
        class ViewportContext;
    }
#endif

namespace MultiplayerSample
{
    class PlayerIdentityComponent
        : public PlayerIdentityComponentBase
        #if AZ_TRAIT_CLIENT
            , AZ::TickBus::Handler
        #endif
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerSample::PlayerIdentityComponent, s_playerIdentityComponentConcreteUuid, MultiplayerSample::PlayerIdentityComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;


    private:
        #if AZ_TRAIT_CLIENT
            static constexpr float FontScale = 0.7f;

            // AZ::TickBus::Handler overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // Cache properties required for font rendering...
            AZ::RPI::ViewportContextPtr m_viewport;
            AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
            AzFramework::TextDrawParameters m_drawParams;
        #endif
    };


    class PlayerIdentityComponentController
        : public PlayerIdentityComponentControllerBase
        , public PlayerIdentityRequestBus::Handler
    {
    public:
        explicit PlayerIdentityComponentController(PlayerIdentityComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

#if AZ_TRAIT_SERVER
        void HandleRPC_AssignPlayerName(AzNetworking::IConnection* invokingConnection, const PlayerNameString& newPlayerName) override;
        void HandleRPC_ResetPlayerState(AzNetworking::IConnection* invokingConnection, const PlayerResetOptions& resetOptions) override;
#endif

        // PlayerIdentityRequestBus overrides ...
        const char* GetPlayerIdentityName() override;

    private:
        AZ::Event<PlayerNameString>::Handler m_onAutomonousPlayerNameChanged{ [](const PlayerNameString& playerName)
        {
            PlayerIdentityNotificationBus::Broadcast(&PlayerIdentityNotifications::OnAutonomousPlayerNameChanged, playerName.c_str());;
        } };
    };
}
