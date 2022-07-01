/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include "AWSMetricsSubmissionComponent.h"

#include <Multiplayer/IMultiplayer.h>
#include <AWSMetricsBus.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace MultiplayerSample
{
    using namespace Multiplayer;

    AZ_CVAR(bool, sv_submit_aws_metrics, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to submit metrics to the AWS backend");

    void AWSMetricsSubmissionComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serializationContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializationContext->Class<AWSMetricsSubmissionComponent, Component>()
                ->Field("AWS Metrics Submission Interval in Seconds", &AWSMetricsSubmissionComponent::m_AWSMetricsSubmissionIntervalInSec)
                ->Version(1);

            if (const auto editContext = serializationContext->GetEditContext())
            {
                editContext->Class<AWSMetricsSubmissionComponent>(AWSMetricsSubmissionComponentName,
                    "Handles metrics submission via the AWSMetrics Gem")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "MultiplayerSample")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(nullptr, &AWSMetricsSubmissionComponent::m_AWSMetricsSubmissionIntervalInSec, "AWS Metrics Submission Interval in Seconds",
                        "Time interval for submitting metrics. "
                        "This value only applies to metrics which are required to be submitted periodically, like client connection counts.")
                    ;
            }
        }
    }

    void AWSMetricsSubmissionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE(AWSMetricsSubmissionComponentName));
    }

    void AWSMetricsSubmissionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE(AWSMetricsSubmissionComponentName));
    }

    void AWSMetricsSubmissionComponent::Activate()
    {
        if (!sv_submit_aws_metrics)
        {
            return;
        }

        RegisterPlayerConnectionMetrics();

        AZ::TickBus::Handler::BusConnect();

        AWSMetrics::AWSMetricsNotificationBus::Handler::BusConnect();
    }

    void AWSMetricsSubmissionComponent::Deactivate()
    {
        if (!sv_submit_aws_metrics)
        {
            return;
        }

        AWSMetrics::AWSMetricsNotificationBus::Handler::BusDisconnect();

        AZ::TickBus::Handler::BusDisconnect();

        AWSMetrics::AWSMetricsRequestBus::Broadcast(&AWSMetrics::AWSMetricsRequests::FlushMetrics);
    }

    void AWSMetricsSubmissionComponent::RegisterPlayerConnectionMetrics()
    {
        // Add a handler for the ConnectionAcquired event to submit the client_join metrics
        m_connectHandler = ConnectionAcquiredEvent::Handler([](MultiplayerAgentDatum)
            {
                AZStd::vector<AWSMetrics::MetricsAttribute> clientJointMetricsAttributes;
                clientJointMetricsAttributes.emplace_back(AWSMetrics::MetricsAttribute(MetricsEventNameAttributeKey, ClientJoinMetricsEvent));

                bool result = false;
                AWSMetrics::AWSMetricsRequestBus::BroadcastResult(result, &AWSMetrics::AWSMetricsRequests::SubmitMetrics, clientJointMetricsAttributes, 0, MetricsEventSource, true);
                if (!result)
                {
                    AZ_TracePrintf(AWSMetricsSubmissionComponentName, "Failed to submit the %s metrics", ClientJoinMetricsEvent);
                }
            });
        AZ::Interface<IMultiplayer>::Get()->AddConnectionAcquiredHandler(m_connectHandler);

        // Add a handler for the EndpointDisonnected event to submit the client_leave metrics
        m_disconnectHandler = EndpointDisonnectedEvent::Handler([](MultiplayerAgentType)
            {
                AZStd::vector<AWSMetrics::MetricsAttribute> clientLeaveMetricsAttributes;
                clientLeaveMetricsAttributes.emplace_back(AWSMetrics::MetricsAttribute(MetricsEventNameAttributeKey, ClientLeaveMetricsEvent));

                bool result = false;
                AWSMetrics::AWSMetricsRequestBus::BroadcastResult(result, &AWSMetrics::AWSMetricsRequests::SubmitMetrics, clientLeaveMetricsAttributes, 0, MetricsEventSource, true);
                if (!result)
                {
                    AZ_TracePrintf(AWSMetricsSubmissionComponentName, "Failed to submit the %s metrics", ClientLeaveMetricsEvent);
                }

            });
        AZ::Interface<IMultiplayer>::Get()->AddEndpointDisonnectedHandler(m_disconnectHandler);
    }

    void AWSMetricsSubmissionComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!AZ::Interface<IMultiplayer>::Get() ||
            AZ::Interface<IMultiplayer>::Get()->GetAgentType() != MultiplayerAgentType::DedicatedServer)
        {
            return;
        }

        m_interval += deltaTime;
        if (m_interval > m_AWSMetricsSubmissionIntervalInSec)
        {
            // Submit the connection count metrics periodically to the AWS backend
            SubmitEndpointConnectionCountMetrics();
            m_interval = 0;
        }
    }

    void AWSMetricsSubmissionComponent::SubmitEndpointConnectionCountMetrics()
    {
        MultiplayerStats& stats = AZ::Interface<IMultiplayer>::Get()->GetStats();

        AZStd::vector<AWSMetrics::MetricsAttribute> clientConnectCountMetricsAttributes;
        clientConnectCountMetricsAttributes.emplace_back(AWSMetrics::MetricsAttribute(MetricsEventNameAttributeKey, ClientCountMetricsEvent));
        clientConnectCountMetricsAttributes.emplace_back(AWSMetrics::MetricsAttribute(ClientCountMetricsEvent, (int) stats.m_clientConnectionCount));

        bool result = false;
        AWSMetrics::AWSMetricsRequestBus::BroadcastResult(result, &AWSMetrics::AWSMetricsRequests::SubmitMetrics, clientConnectCountMetricsAttributes, 0, MetricsEventSource, false);
        if (!result)
        {
            AZ_TracePrintf(AWSMetricsSubmissionComponentName, "Failed to submit the %s metrics", ClientCountMetricsEvent);
        }
    }

    void AWSMetricsSubmissionComponent::OnSendMetricsSuccess([[maybe_unused]] int requestId)
    {
    }

    void AWSMetricsSubmissionComponent::OnSendMetricsFailure([[maybe_unused]] int requestId, [[maybe_unused]] const AZStd::string& errorMessage)
    {
        AZ_TracePrintf(AWSMetricsSubmissionComponentName, "Failed to sent metrics: %s", errorMessage.c_str());
    }
}
