/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Utils/MPSMatchmakingComponent.h>

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
        AWS_FEATURE_GEM_SERVICE(MPSMatchmakingComponent);

        //! GET request to place a matchmaking request "/requestmatchmaking".
        class MPSRequestMatchmakingRequest
            : public AWSCore::ServiceRequest
        {
        public:
            SERVICE_REQUEST(MPSMatchmakingComponent, HttpMethod::HTTP_GET, "/requestmatchmaking");

            bool UseAWSCredentials()
            {
                return true;
            }

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
            bool ok = true;
            ok = ok && request.WriteJsonBodyParameter(*this);
            return ok;
        }

        bool MPSRequestMatchmakingRequest::Parameters::WriteJson(AWSCore::JsonWriter& writer) const
        {
            bool ok = true;
            ok = ok && writer.Write(latencies);
            return ok;
        }
    }

    void MPSMatchmakingComponent::Activate()
    {
        MPSMatchmakingComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MPSMatchmakingComponent::Deactivate()
    {
        MPSMatchmakingComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void MPSMatchmakingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MPSMatchmakingComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MPSMatchmakingComponent>("MPSMatchmakingComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                 ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MPSMatchmakingComponent>("MPSMatchmakingComponent Group")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ;
        
            behaviorContext->EBus<MPSMatchmakingComponentRequestBus>("MPSMatchmakingComponentRequests")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Event("RequestMatch", &MPSMatchmakingComponentRequestBus::Events::RequestMatch)
                ->Event("HasMatch", &MPSMatchmakingComponentRequestBus::Events::HasMatch)
                ;
        }
    }

    void MPSMatchmakingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MPSMatchmakingService"));
    }

    void MPSMatchmakingComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MPSMatchmakingService"));
    }

    void MPSMatchmakingComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MPSMatchmakingComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void SetApiEndpointAndRegion(MPSGameLift::ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config)
    {
        AZStd::string actualRegion;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(actualRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);

        AZStd::string restApi;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(restApi, &AWSCore::AWSResourceMappingRequests::GetResourceNameId, "MPSMatchamking");
        config->region = actualRegion.c_str();
        config->endpointOverride =  AZStd::string::format("https://%s.execute-api.%s.amazonaws.com/%s",
            restApi.c_str(), actualRegion.c_str(), "Prod/requestmatchmaking").c_str();
    }

   bool MPSMatchmakingComponent::RequestMatch([[maybe_unused]] const AZStd::string& latencies)
    {
        if (m_ticketId != "")
        {
            AZ_Warning("MPSMatchmakingComponent", false, "Ticket already exists %s", m_ticketId.c_str())
            return true;
        }
        MPSGameLift::ServiceAPI::MPSRequestMatchmakingRequestJob::Config* config = ServiceAPI::MPSRequestMatchmakingRequestJob::GetDefaultConfig();

        // Setup example game service endpoint
        SetApiEndpointAndRegion(config);

        // TODO: Need to generate latency string and send in request
        ServiceAPI::MPSRequestMatchmakingRequestJob* requestJob = ServiceAPI::MPSRequestMatchmakingRequestJob::Create(
            []([[maybe_unused]] ServiceAPI::MPSRequestMatchmakingRequestJob* successJob)
            {
                AZ_Printf("MPSMatchmakingComponent", "Request for matchmaking made");
                // TODO: Grab TicketId from success job

            },
            []([[maybe_unused]] ServiceAPI::MPSRequestMatchmakingRequestJob* failJob)
            {
                AZ_Error("MPSMatchmakingComponent", false, "Unable to request match error: %s", failJob->error.message.c_str());
            },
            config);

        requestJob->parameters.latencies = latencies;
        requestJob->Start();
        return true;
    }

    bool MPSMatchmakingComponent::HasMatch(const AZStd::string& ticketId)
    {
        if (ticketId == "")
        {
            return false;
        }
        return false;
    }





}
#pragma optimize("",on)