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
    class BubbleProjectileRequests : public AZ::EBusTraits
    {
    public:
        virtual ~BubbleProjectileRequests() = default;

        virtual void SetBubbleProperties(AZ::EntityId instigator) = 0;
    };

    using BubbleProjectileRequestBus = AZ::EBus<BubbleProjectileRequests>;
}
