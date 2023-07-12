/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <MPSGameLift/MPSLatencyComponentInterface.h>

namespace MPSGameLift
{
    class MPSMatchmakingComponent
        : public AZ::Component
        , public MPSMatchmakingComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MPSMatchmakingComponent, "{BF5F9343-63B5-4703-89ED-9CDBF4FE6004}");
        static void Reflect(AZ::ReflectContext* context);

        // MPSMatchmakingComponentRequestBus::Handler overrides...
        bool RequestMatch(const AZStd::string& latencies) override;
        AZStd::string GetTicketId() const override {return m_ticketId;}
        bool HasMatch(const AZStd::string& ticketId) override;

     protected:
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::string m_ticketId;
     };
}
