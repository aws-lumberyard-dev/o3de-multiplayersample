/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AWSMetricsBus.h>

using namespace Multiplayer;

namespace MultiplayerSample
{
    static constexpr char AWSMetricsSubmissionComponentName[] = "AWSMetricsSubmissionComponent";
    static constexpr char ClientJoinMetricsEvent[] = "client_join";
    static constexpr char ClientLeaveMetricsEvent[] = "client_leave";
    static constexpr char ClientCountMetricsEvent[] = "client_connection_count";
    static constexpr char MetricsEventNameAttributeKey[] = "event_name";
    static constexpr char MetricsEventSource[] = "MultiplayerSample";

    //! Component for submitting metrics to the AWS backend
    class AWSMetricsSubmissionComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AWSMetrics::AWSMetricsNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AWSMetricsSubmissionComponent, "{F16A5A09-C351-4862-8CCA-3E8419123538}", Component);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // TickBus implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AWSMetricsNotificationBus implementation
        void OnSendMetricsSuccess(int requestId) override;
        void OnSendMetricsFailure(int requestId, const AZStd::string& errorMessage) override;

    private:
        //! Connect handlers to the player connection events for submitting client_join and client_leave metrics
        void RegisterPlayerConnectionMetrics();
        //! Submit the client_connection_count metrics
        void SubmitEndpointConnectionCountMetrics();

        ConnectionAcquiredEvent::Handler m_connectHandler;
        EndpointDisonnectedEvent::Handler m_disconnectHandler;

        //! Time interval for submitting metrics.
        //! This value only applies to metrics which are required to be submitted periodically, like client connection counts.
        float m_AWSMetricsSubmissionIntervalInSec=60.0;

        float m_interval = 0;
    };
}
