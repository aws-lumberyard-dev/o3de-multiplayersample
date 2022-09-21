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

        void ApplyJumpPadEffect(const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration);
    };


    class PlayerJumpPadEffectComponentController
        : public PlayerJumpPadEffectComponentControllerBase
    {
    public:
        explicit PlayerJumpPadEffectComponentController(PlayerJumpPadEffectComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        void ApplyJumpPadEffect(const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration);

        void ProcessInput(Multiplayer::NetworkInput& networkInput, float deltaTime) override;

    private:
        void JumpPadEffectTick();
        AZ::ScheduledEvent m_jumpPadEffectEvent{ [this]()
        {
            JumpPadEffectTick();
        }, AZ::Name("PlayerJumpPadEffect") };

        AZ::TimeMs m_jumpPadEffectDuration = AZ::Time::ZeroTimeMs;
    };
}
