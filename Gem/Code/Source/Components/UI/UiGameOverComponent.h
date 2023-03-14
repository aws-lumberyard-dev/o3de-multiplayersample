/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Component/Component.h>

#include <UiGameOverBus.h>

namespace MultiplayerSample
{
    class UiGameOverComponent
        : public AZ::Component
        , public UiGameOverBus::Handler
    {
    public:
        AZ_COMPONENT(UiGameOverComponent, "{37a2de13-a8fa-4ee1-8652-e17253137f62}");

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;

        //! UiGameOverBus overrides
        //! @{
        void SetGameOverScreenEnabled(bool enabled) override;
        void DisplayResults(MatchResultsSummary results) override;
        //! }@

    private:
        AZ::EntityId m_gameOverRootElement;
        AZ::EntityId m_winnerNameElement;
        AZ::EntityId m_matchResultsElement;

        AZStd::string BuildResultsSummary(const AZStd::vector<PlayerState>& playerStates);
    };
}