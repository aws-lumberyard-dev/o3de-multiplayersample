/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BubbleProjectileSpawnerBus.h>
#include <Source/AutoGen/BubbleProjectileSpawnerComponent.AutoComponent.h>

namespace MultiplayerSample
{
    class BubbleProjectileSpawnerComponentController
        : public BubbleProjectileSpawnerComponentControllerBase
        , public BubbleProjectileSpawnerRequestBus::Handler
    {
    public:
        explicit BubbleProjectileSpawnerComponentController(BubbleProjectileSpawnerComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        // BubbleProjectileSpawnerRequestBus
        void LaunchBubbleProjectile(const AZ::Transform& from, AZ::EntityId byPlayer) override;
    };
}
