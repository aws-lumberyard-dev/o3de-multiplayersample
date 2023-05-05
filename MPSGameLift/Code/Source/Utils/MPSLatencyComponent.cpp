
#include <Utils/MPSLatencyComponent.h>

#include <aws/core/http/HttpResponse.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include <HttpRequestor/HttpTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#pragma optimize("",off)
namespace MPSGameLift
{
    void MPSLatencyComponent::Activate()
    {
        MPSLatencyComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MPSLatencyComponent::Deactivate()
    {
        MPSLatencyComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    void MPSLatencyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MPSLatencyComponent, AZ::Component>()
                ->Version(1)
                ->Field("Regions", &MPSLatencyComponent::m_regions_)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MPSLatencyComponent>("MPSLatencyComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                ->DataElement(AZ::Edit::UIHandlers::Default, &MPSLatencyComponent::m_regions_, "AWS Regions", "List of AWS Regions")
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MPSLatencyComponent>("MPSLatencyComponent Group")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ;
        
            behaviorContext->EBus<MPSLatencyComponentRequestBus>("MPSLatencyComponentRequests")
                ->Attribute(AZ::Script::Attributes::Category, "MPSGameLift Gem")
                ->Event("RequestLatencies", &MPSLatencyComponentRequestBus::Events::RequestLatencies)
                ->Event("HasLatencies", &MPSLatencyComponentRequestBus::Events::HasLatencies)
                ->Event("GetLatencyForRegion", &MPSLatencyComponentRequestBus::Events::GetLatencyForRegion)
                ;
        }
    }

    void MPSLatencyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MPSLatencyComponentService"));
    }

    void MPSLatencyComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MPSLatencyComponentService"));
    }

    void MPSLatencyComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MPSLatencyComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void MPSLatencyComponent::RequestLatencies()
    {
        // First request just to warm up the TCP connections
        HttpRequestor::HttpRequestorRequestBus::Broadcast(
    &HttpRequestor::HttpRequestorRequests::AddRequest, "https://dynamodb.us-east-1.amazonaws.com", Aws::Http::HttpMethod::HTTP_GET,
    [](const Aws::Utils::Json::JsonView& data, Aws::Http::HttpResponseCode responseCode, const AZStd::string& uuid, uint32_t duration)
    {
        if (responseCode == Aws::Http::HttpResponseCode::OK)
        {
            AZ_Trace("MPSLatencyComponent",  "Call (%s) succeed with %s %d in %dmS", 
                uuid.c_str(), data.WriteCompact().c_str(), responseCode, duration);
        }
        else
        {
            AZ_Info("MPSLatencyComponent", "Call (%s) Failed in %d", uuid.c_str(), duration);
        }
    });

        HttpRequestor::HttpRequestorRequestBus::Broadcast(
    &HttpRequestor::HttpRequestorRequests::AddRequest, "https://dynamodb.us-east-1.amazonaws.com", Aws::Http::HttpMethod::HTTP_GET,
    [this](const Aws::Utils::Json::JsonView& data, Aws::Http::HttpResponseCode responseCode, const AZStd::string& uuid, uint32_t duration)
    {
        if (responseCode == Aws::Http::HttpResponseCode::OK)
        {
            AZ_Trace("MPSLatencyComponent",  "Call (%s) succeed with %s %d in %d mS", 
                uuid.c_str(), data.WriteCompact().c_str(), responseCode, duration);
        }
        this->SetLatencyForRegion("us-east-1", duration);
    });

        HttpRequestor::HttpRequestorRequestBus::Broadcast(
    &HttpRequestor::HttpRequestorRequests::AddRequest, "https://dynamodb.us-west-2.amazonaws.com", Aws::Http::HttpMethod::HTTP_GET,
    [this](const Aws::Utils::Json::JsonView& data, Aws::Http::HttpResponseCode responseCode, const AZStd::string& uuid, uint32_t duration)
    {
        if (responseCode == Aws::Http::HttpResponseCode::OK)
        {
            AZ_Trace("MPSLatencyComponent",  "Call (%s) succeed with %s %d in %d mS", 
                uuid.c_str(), data.WriteCompact().c_str(), responseCode, duration);
        }
        this->SetLatencyForRegion("us-west-2", duration);
        m_latenciesAvailable.Signal(true);
    });

    }

    void MPSLatencyComponent::SetLatencyForRegion(const AZStd::string& region, uint32_t latency)
    {
        m_latencyMap_[region] = latency;
    }

    bool MPSLatencyComponent::HasLatencies() const
    {
        bool ok = true;
        for (auto iter = m_latencyMap_.begin(); iter != m_latencyMap_.end(); ++iter)
        {
            if (iter->second == 0)
            {
                // latency not set
                ok = false;
                break;
            }
        }
        return ok;
    }


    uint32_t MPSLatencyComponent::GetLatencyForRegion(const AZStd::string& region) const
    {
        if (const auto pos = m_latencyMap_.find(region); pos != m_latencyMap_.end())
        {
            return pos->second;
        }
        else {
            return 150;
        }
    }
} // namespace MPSGameLift
#pragma optimize("",on)