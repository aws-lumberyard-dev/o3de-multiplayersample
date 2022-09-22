/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BubbleProjectileBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <Source/AutoGen/BubbleProjectileComponent.AutoComponent.h>

namespace MultiplayerSample
{
    class BubbleProjectileComponentController
        : public BubbleProjectileComponentControllerBase
        , public BubbleProjectileRequestBus::Handler
    {
    public:
        explicit BubbleProjectileComponentController(BubbleProjectileComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        // BubbleProjectileRequestBus
        void SetBubbleProperties(AZ::EntityId instigator) override;

    private:
        void HideEnergyBall();
        void TryKnockbackPlayer(AZ::Entity* target);

        void OnCollisionBegin(const AzPhysics::CollisionEvent& collisionEvent);
        AzPhysics::SimulatedBodyEvents::OnCollisionBegin::Handler m_collisionHandler{ [this](
            AzPhysics::SimulatedBodyHandle, const AzPhysics::CollisionEvent& collisionEvent)
        {
            OnCollisionBegin(collisionEvent);
        } };

        void LoadEnergyBallSettings();
        AZ::Vector3 m_direction = AZ::Vector3::CreateZero();
        double m_knockbackDistance = 0.0;
        double m_speed = 0.0;
        AZ::s64 m_armorDamage = 0;
    };
}
