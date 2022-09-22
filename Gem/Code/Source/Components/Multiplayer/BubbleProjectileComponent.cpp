/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MultiplayerSampleTypes.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Source/AutoGen/NetworkHealthComponent.AutoComponent.h>
#include <Source/Components/Multiplayer/BubbleProjectileComponent.h>
#include <Source/Components/Multiplayer/PlayerKnockbackEffectComponent.h>

namespace MultiplayerSample
{
    BubbleProjectileComponentController::BubbleProjectileComponentController(BubbleProjectileComponent& parent)
        : BubbleProjectileComponentControllerBase(parent)
    {
    }

    void BubbleProjectileComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        LoadEnergyBallSettings();

        AzPhysics::RigidBody* body = nullptr;
        Physics::RigidBodyRequestBus::EventResult(body, GetEntityId(), &Physics::RigidBodyRequests::GetRigidBody);
        if (body)
        {
            body->RegisterOnCollisionBeginHandler(m_collisionHandler);
        }
    }

    void BubbleProjectileComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        m_collisionHandler.Disconnect();
    }

    void BubbleProjectileComponentController::SetBubbleProperties([[maybe_unused]] AZ::EntityId instigator)
    {
    }

    //void BubbleProjectileComponentController::HandleRPC_LaunchBall([[maybe_unused]] AzNetworking::IConnection* invokingConnection,
    //                                                               const AZ::Vector3& startingPosition, const AZ::Vector3& direction)
    //{
    //    m_direction = direction;

    //    using RigidBodyBus = Physics::RigidBodyRequestBus;
    //    RigidBodyBus::Event(GetEntityId(), &RigidBodyBus::Events::DisablePhysics);

    //    // move self and increment resetCount to prevent transform interpolation
    //    AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, startingPosition);
    //    GetNetworkTransformComponentController()->ModifyResetCount()++;

    //    RigidBodyBus::Event(GetEntityId(), &RigidBodyBus::Events::EnablePhysics);
    //    RigidBodyBus::Event(GetEntityId(), &RigidBodyBus::Events::SetLinearVelocity, direction * aznumeric_cast<float>(m_speed));
    //}

    void BubbleProjectileComponentController::HideEnergyBall()
    {
        Physics::RigidBodyRequestBus::Event(GetEntityId(), &Physics::RigidBodyRequestBus::Events::DisablePhysics);

        // move self and increment resetCount to prevent transform interpolation
        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTranslation, AZ::Vector3::CreateAxisZ(-1000.f));
        GetNetworkTransformComponentController()->ModifyResetCount()++;
    }

    void BubbleProjectileComponentController::OnCollisionBegin(const AzPhysics::CollisionEvent& collisionEvent)
    {
        if (collisionEvent.m_body2)
        {
            AZ::Entity* target = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(target,
                &AZ::ComponentApplicationBus::Events::FindEntity, collisionEvent.m_body2->GetEntityId());
            if (target)
            {
                if (NetworkHealthComponent* health = target->FindComponent<NetworkHealthComponent>())
                {
                    health->SendHealthDelta(aznumeric_cast<float>(-m_armorDamage));
                }

                TryKnockbackPlayer(target);
            }

            HideEnergyBall();
        }
    }

    void BubbleProjectileComponentController::LoadEnergyBallSettings()
    {
        if (const auto registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(m_knockbackDistance, KnockbackDistanceByEnergyBallSetting);
            registry->Get(m_speed, EnergyBallSpeedSetting);
            registry->Get(m_armorDamage, EnergyBallArmorDamageSetting);
        }
    }

    void BubbleProjectileComponentController::TryKnockbackPlayer(AZ::Entity* target)
    {
        if (PlayerKnockbackEffectComponent* effect = target->FindComponent<PlayerKnockbackEffectComponent>())
        {
            const AZ::Vector3 direction = m_direction.GetNormalized();
            if (m_knockbackDistance > 0.0)
            {
                effect->RPC_Knockback(direction * aznumeric_cast<float>(m_knockbackDistance));
            }
        }
    }
}
