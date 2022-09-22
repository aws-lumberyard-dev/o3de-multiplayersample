/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MultiplayerSampleTypes.h>
#include <AzCore/EBus/EBus.h>

namespace MultiplayerSample
{
    class BubbleProjectileSpawnerRequests : public AZ::EBusTraits
    {
    public:
        virtual ~BubbleProjectileSpawnerRequests() = default;

        virtual void LaunchBubbleProjectile(const AZ::Transform& from, AZ::EntityId byPlayer) = 0;
    };

    using BubbleProjectileSpawnerRequestBus = AZ::EBus<BubbleProjectileSpawnerRequests>;
}
