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


    PlayerJumpPadEffectComponentController::PlayerJumpPadEffectComponentController(PlayerJumpPadEffectComponent& parent)
        : PlayerJumpPadEffectComponentControllerBase(parent)
    {
    }

    void PlayerJumpPadEffectComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void PlayerJumpPadEffectComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_jumpPadEffectEvent.RemoveFromQueue();
    }

    void PlayerJumpPadEffectComponentController::HandleRPC_ApplyJumpPadEffect(
        [[maybe_unused]] AzNetworking::IConnection* invokingConnection, 
        const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration)
    {
        AZ_Printf(__FUNCTION__, "");

        GetNetworkTransformComponentController()->ModifyResetCount()++;
        RPC_StartJump(jumpVelocity, effectDuration);
    }

    void PlayerJumpPadEffectComponentController::HandleRPC_StartJump([[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        const AZ::Vector3& jumpVelocity, const AZ::TimeMs& effectDuration)
    {
        AZ_Printf(__FUNCTION__, "");

        m_isUnderJumpPadEffect = true;
        m_jumpPadVector = jumpVelocity;
        m_jumpPadEffectEvent.Enqueue(effectDuration);
    }

    void PlayerJumpPadEffectComponentController::JumpPadEffectTick()
    {
        AZ_Printf(__FUNCTION__, "Jump effect is over");

        // Jump effect is over
        m_isUnderJumpPadEffect = false;
    }

    void PlayerJumpPadEffectComponentController::CreateInput([[maybe_unused]] Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        auto* componentInput = input.FindComponentInput<PlayerJumpPadEffectComponentNetworkInput>();
        if (componentInput)
        {
            componentInput->m_jumpPadEffect = m_isUnderJumpPadEffect;
            componentInput->m_jumpPadVector = m_jumpPadVector;
        }
    }

    void PlayerJumpPadEffectComponentController::ProcessInput([[maybe_unused]] Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        const auto* componentInput = input.FindComponentInput<PlayerJumpPadEffectComponentNetworkInput>();
        if (componentInput)
        {
            if (componentInput->m_jumpPadEffect)
            {
                GetNetworkCharacterComponentController()->TryMoveWithVelocity(componentInput->m_jumpPadVector, deltaTime);
            }
            else // apply gravity
            {                
                const AZ::Vector3 gravity = AZ::Vector3::CreateAxisZ(-GetGravity());
                GetNetworkCharacterComponentController()->TryMoveWithVelocity(gravity, deltaTime);
            }
        }
    }
}
