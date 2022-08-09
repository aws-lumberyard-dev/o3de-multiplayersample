/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <MultiplayerSampleTypes.h>

namespace MultiplayerSample
{
    class UiGameOverNotifications : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(UiGameOverNotifications, "{56c20d63-9ac9-481e-8b0d-3767a2112e3d}");
        virtual ~UiGameOverNotifications() = default;

        virtual void SetGameOverScreenEnabled([[maybe_unused]] bool enabled) = 0;
        virtual void DisplayResults(MatchResultsSummary results) {}
    };

    using UiGameOverBus = AZ::EBus<UiGameOverNotifications>;
}