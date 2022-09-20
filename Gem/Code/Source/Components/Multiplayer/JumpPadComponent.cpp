/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <Source/Components/Multiplayer/JumpPadComponent.h>

#include "PlayerJumpPadEffectComponent.h"

namespace MultiplayerSample
{
    void JumpPadComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<JumpPadComponent, JumpPadComponentBase>()
                ->Version(1);
        }
        JumpPadComponentBase::Reflect(context);
    }

    void JumpPadComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        JumpPadComponentBase::GetRequiredServices(required);
        required.push_back(AZ_CRC_CE("PhysicsTriggerService"));
    }

    void JumpPadComponent::OnInit()
    {
    }

    void JumpPadComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void JumpPadComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }


    JumpPadComponentController::JumpPadComponentController(JumpPadComponent& parent)
        : JumpPadComponentControllerBase(parent)
    {
    }

    void JumpPadComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        if (physicsSystem)
        {
            const AZ::EntityId selfId = GetEntity()->GetId();
            auto [sceneHandle, bodyHandle] = physicsSystem->FindAttachedBodyHandleFromEntityId(selfId);
            AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(
                sceneHandle, bodyHandle, m_enterTrigger);
        }
    }

    void JumpPadComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_enterTrigger.Disconnect();
    }

    void JumpPadComponentController::OnTriggerEnter([[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
        const AzPhysics::TriggerEvent& triggerEvent)
    {
        if (triggerEvent.m_otherBody)
        {
            AZ::Entity* collidingEntity = nullptr;
            AZ::EntityId collidingEntityId = triggerEvent.m_otherBody->GetEntityId();
            AZ::ComponentApplicationBus::BroadcastResult(collidingEntity,
                &AZ::ComponentApplicationBus::Events::FindEntity, collidingEntityId);
            if (collidingEntity)
            {
                if (PlayerJumpPadEffectComponent* effect = collidingEntity->FindComponent<PlayerJumpPadEffectComponent>())
                {
                    const AZ::Quaternion worldQuaternion = GetEntity()->GetTransform()->GetWorldRotationQuaternion();
                    const AZ::Vector3 jumpVelocity = worldQuaternion.TransformVector(AZ::Vector3::CreateAxisY(GetJumpVelocity()));

                    effect->RPC_ApplyJumpPadEffect(jumpVelocity, GetJumpDuration());
                }
            }
        }
    }
}
