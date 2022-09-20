/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetworkCharacterComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Source/Components/Multiplayer/PlayerJumpPadEffectComponent.h>

namespace MultiplayerSample
{
    void PlayerJumpPadEffectComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PlayerJumpPadEffectComponent, PlayerJumpPadEffectComponentBase>()
                ->Version(1);
        }
        PlayerJumpPadEffectComponentBase::Reflect(context);
    }

    void PlayerJumpPadEffectComponent::OnInit()
    {
    }

    void PlayerJumpPadEffectComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void PlayerJumpPadEffectComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void PlayerJumpPadEffectComponent::ApplyJumpPadEffect(const AZ::Vector3& jumpVelocity,
        const AZ::TimeMs& effectDuration)
    {
        if (HasController())
        {
            static_cast<PlayerJumpPadEffectComponentController*>(GetController())->ApplyJumpPadEffect(jumpVelocity, effectDuration);
        }
    }


    PlayerJumpPadEffectComponentController::PlayerJumpPadEffectComponentController(PlayerJumpPadEffectComponent& parent)
        : PlayerJumpPadEffectComponentControllerBase(parent)
    {
    }

    void PlayerJumpPadEffectComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_jumpPadEffectEvent.Enqueue(AZ::Time::ZeroTimeMs, true);
    }

    void PlayerJumpPadEffectComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_jumpPadEffectEvent.RemoveFromQueue();
    }

    void PlayerJumpPadEffectComponentController::ApplyJumpPadEffect(
        const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration)
    {
        GetNetworkTransformComponentController()->ModifyResetCount()++;

        SetIsJumpPadEffectInProgress(true);
        SetJumpPadVector(jumpVelocity);
        SetJumpPadEffectDuration(effectDuration);
    }

    void PlayerJumpPadEffectComponentController::JumpPadEffectTick()
    {
        const AZ::TimeMs deltaMs = m_jumpPadEffectEvent.TimeInQueueMs();
        const float deltaTime = AZ::TimeMsToSeconds(deltaMs);

        if (GetJumpPadEffectDuration() > AZ::Time::ZeroTimeMs)
        {
            ModifyJumpPadEffectDuration() -= deltaMs;
            if (GetJumpPadEffectDuration() <= AZ::Time::ZeroTimeMs)
            {
                // Jump effect is over
                SetIsJumpPadEffectInProgress(false);
                SetJumpPadEffectDuration(AZ::Time::ZeroTimeMs);
            }
        }

        if (GetIsJumpPadEffectInProgress())
        {
            GetNetworkCharacterComponentController()->TryMoveWithVelocity(GetJumpPadVector(), deltaTime);
        }
        else // apply gravity
        {
            const AZ::Vector3 gravity = AZ::Vector3::CreateAxisZ(-GetGravity());
            GetNetworkCharacterComponentController()->TryMoveWithVelocity(gravity, deltaTime);
        }
    }
}
