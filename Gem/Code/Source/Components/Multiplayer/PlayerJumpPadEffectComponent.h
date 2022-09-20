/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/PlayerJumpPadEffectComponent.AutoComponent.h>

namespace MultiplayerSample
{
    class PlayerJumpPadEffectComponent
        : public PlayerJumpPadEffectComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(MultiplayerSample::PlayerJumpPadEffectComponent, s_playerJumpPadEffectComponentConcreteUuid, MultiplayerSample::PlayerJumpPadEffectComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
    };


    class PlayerJumpPadEffectComponentController
        : public PlayerJumpPadEffectComponentControllerBase
    {
    public:
        explicit PlayerJumpPadEffectComponentController(PlayerJumpPadEffectComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        //! Common input creation logic for the NetworkInput.
        //! Fill out the input struct and the MultiplayerInputDriver will send the input data over the network
        //!    to ensure it's processed.
        //! @param input  input structure which to store input data for sending to the authority
        //! @param deltaTime amount of time to integrate the provided inputs over
        void CreateInput(Multiplayer::NetworkInput& input, float deltaTime) override;

        //! Common input processing logic for the NetworkInput.
        //! @param input  input structure to process
        //! @param deltaTime amount of time to integrate the provided inputs over
        void ProcessInput(Multiplayer::NetworkInput& input, float deltaTime) override;

        void HandleRPC_ApplyJumpPadEffect(AzNetworking::IConnection* invokingConnection, const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration) override;
        void HandleRPC_StartJump(AzNetworking::IConnection* invokingConnection, const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration) override;

    private:
        bool m_isUnderJumpPadEffect = false;
        AZ::Vector3 m_jumpPadVector = AZ::Vector3::CreateZero();
        
        void JumpPadEffectTick();
        AZ::ScheduledEvent m_jumpPadEffectEvent{ [this]()
        {
            JumpPadEffectTick();
        }, AZ::Name("PlayerJumpPadEffect") };
    };
}
