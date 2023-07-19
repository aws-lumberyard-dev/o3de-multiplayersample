/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MatchmakingSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <AWSCoreBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <Framework/ServiceRequestJob.h>

#pragma optimize("",off)
namespace MPSGameLift
{
    namespace ServiceAPI
    {
        //! Struct for storing the success response.
        struct MPSRequestMatchmakingSuccessResponse
        {
            //! Identify the expected property type and provide a location where the property value can be stored.
            //! @param key Name of the property.
            //! @param reader JSON reader to read the property.
            bool OnJsonKey(const char* key, AWSCore::JsonReader& reader);

            AZStd::string result; //!< Processing result for the input record.
        };

        // Service RequestJobs
        AWS_FEATURE_GEM_SERVICE(MatchmakingSystemComponent);

        //! GET request to place a matchmaking request "/requestmatchmaking".
        class MPSRequestMatchmakingRequest
            : public AWSCore::ServiceRequest
        {
        public:
            SERVICE_REQUEST(MatchmakingSystemComponent, HttpMethod::HTTP_GET, "");

            //! Request body for the service API request.
            struct Parameters
            {
                //! Build the service API request.
                //! @request Builder for generating the request.
                //! @return Whether the request is built successfully.
                bool BuildRequest(AWSCore::RequestBuilder& request);

                //! Write to the service API request body.
                //! @param writer JSON writer for the serialization.
                //! @return Whether the serialization is successful.
                bool WriteJson(AWSCore::JsonWriter& writer) const;

                AZStd::string latencies;
            };

            MPSRequestMatchmakingSuccessResponse result;
            Parameters parameters; //! Request parameter.
        };

        using MPSRequestMatchmakingRequestJob = AWSCore::ServiceRequestJob<MPSRequestMatchmakingRequest>;


        constexpr char MPSMatchmakingRequestResultResponseKey[] = "statusCode";

        bool MPSRequestMatchmakingSuccessResponse::OnJsonKey(const char* key, AWSCore::JsonReader& reader)
        {
            if (strcmp(key, MPSMatchmakingRequestResultResponseKey) == 0)
            {
                return reader.Accept(result);
            }
            return reader.Ignore();
        }

        bool MPSRequestMatchmakingRequest::Parameters::BuildRequest(AWSCore::RequestBuilder& request)
        {
            return request.WriteJsonBodyParameter(*this);
        }

        bool MPSRequestMatchmakingRequest::Parameters::WriteJson([[maybe_unused]] AWSCore::JsonWriter& writer) const
        {
            return true;
        }
    }

    void MatchmakingSystemComponent::Activate()
    {
        AZ::Interface<IMatchmaking>::Register(this);
    }

    void MatchmakingSystemComponent::Deactivate()
    {
        AZ::Interface<IMatchmaking>::Unregister(this);
    }

    void MatchmakingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MatchmakingSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MatchmakingSystemComponent>("MatchmakingSystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                 ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MatchmakingSystemComponent>("MatchmakingSystemComponent")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Method("RequestMatch", [](const AZStd::string& latencies)
                    {
                        if (const auto matchmaking = AZ::Interface<IMatchmaking>::Get())
                        {
                            return matchmaking->RequestMatch(latencies);
                        }
                        return false;
                    })
                ->Method("HasMatch", [](const AZStd::string& ticketId)
                    {
                        if (const auto matchmaking = AZ::Interface<IMatchmaking>::Get())
                        {
                            return matchmaking->HasMatch(ticketId);
                        }
                        return false;
                    })
                ;
        }
    }

    void MatchmakingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MPSGameLiftMatchmaking"));
    }

    void SetApiEndpointAndRegion(ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config, const AZStd::string& latencies)
    {
        AZStd::string actualRegion;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);

        AZStd::string restApi;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(restApi, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, "MPSMatchmaking");
        config->region = actualRegion.c_str();
        config->endpointOverride = AZStd::string::format("https://%s.execute-api.%s.amazonaws.com/%s?latencies=%s",
            restApi.c_str(), actualRegion.c_str(), "Prod/requestmatchmaking", latencies.c_str()).c_str();
    }

   bool MatchmakingSystemComponent::RequestMatch(const AZStd::string& latencies)
    {
        if (!m_ticketId.empty())
        {
            AZ_Warning("MatchmakingSystemComponent", false, "Ticket already exists %s", m_ticketId.c_str())
            return true;
        }
        ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config = ServiceAPI::MPSRequestMatchmakingRequestJob::GetDefaultConfig();

        // Setup example game service endpoint
        SetApiEndpointAndRegion(config, latencies);

        ServiceAPI::MPSRequestMatchmakingRequestJob* requestJob = ServiceAPI::MPSRequestMatchmakingRequestJob::Create(
            []([[maybe_unused]] ServiceAPI::MPSRequestMatchmakingRequestJob* successJob)
            {
                // TODO: Grab TicketId from success job
                AZ_Printf("MatchmakingSystemComponent", "Request for matchmaking made: %s", successJob->result.result.c_str());
            },
            []([[maybe_unused]] ServiceAPI::MPSRequestMatchmakingRequestJob* failJob)
            {
                AZ_Error("MatchmakingSystemComponent", false, "Unable to request match error: %s", failJob->error.message.c_str());
            },
            config);

        requestJob->Start();
        return true;
    }

    bool MatchmakingSystemComponent::HasMatch(const AZStd::string& ticketId)
    {
        if (ticketId.empty())
        {
            return false;
        }
        return false;
    }
}
#pragma optimize("",on)
