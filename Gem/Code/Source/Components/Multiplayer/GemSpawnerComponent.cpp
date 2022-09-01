/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Source/Components/NetworkRandomComponent.h>
#include <Source/Components/Multiplayer/GemComponent.h>
#include <Source/Components/Multiplayer/GemSpawnerComponent.h>
#include <Source/Components/PerfTest/NetworkPrefabSpawnerComponent.h>

#pragma optimize("", off)

namespace MultiplayerSample
{
    void GemSpawnable::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GemSpawnable>()
                ->Version(1)
                ->Field("Tag", &GemSpawnable::m_tag)
                ->Field("Asset", &GemSpawnable::m_gemAsset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GemSpawnable>("GemSpawnable", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GemSpawnable::m_tag, "Tag", "Assigned tag for this gem type")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GemSpawnable::m_gemAsset, "Asset", "Spawnable for the gem")
                    ;
            }
        }
    }

    void GemWeightChance::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GemWeightChance>()
                ->Version(1)
                ->Field("Gem Tag Type", &GemWeightChance::m_tag)
                ->Field("Gem Weight", &GemWeightChance::m_weight)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GemWeightChance>("GemWeightChance", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GemWeightChance::m_tag, "Gem Tag Type", "Assigned tag for this gem type")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GemWeightChance::m_weight, "Gem Weight", "Weight value in randomly choosing between the gems")
                    ;
            }
        }
    }

    void RoundSpawnTable::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RoundSpawnTable>()
                ->Version(1)
                ->Field("Gem Weights", &RoundSpawnTable::m_gemWeights)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RoundSpawnTable>("RoundSpawnTable", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RoundSpawnTable::m_gemWeights, "Gem Weights", "Gem weights for a given round")
                    ;
            }
        }
    }

    void GemSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GemSpawnerComponent, GemSpawnerComponentBase>()
                ->Version(1);
        }
        GemSpawnerComponentBase::Reflect(context);
    }


    GemSpawnerComponentController::GemSpawnerComponentController(GemSpawnerComponent& parent)
        : GemSpawnerComponentControllerBase(parent)
    {
    }

    void GemSpawnerComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
    }

    void GemSpawnerComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect();
    }

    void GemSpawnerComponentController::SpawnGems()
    {
        RemoveGems();

        AZ::EBusAggregateResults<AZ::EntityId> aggregator;
        LmbrCentral::TagGlobalRequestBus::EventResult(aggregator, AZ::Crc32(GetGemSpawnTag()),
            &LmbrCentral::TagGlobalRequests::RequestTaggedEntities);

        for (const AZ::EntityId gemSpawnEntity : aggregator.values)
        {
            LmbrCentral::Tags tags;
            LmbrCentral::TagComponentRequestBus::EventResult(tags, gemSpawnEntity,
                &LmbrCentral::TagComponentRequestBus::Events::GetTags);

            const AZ::Crc32 type = ChooseGemType(tags);

            AZ::Vector3 position = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(position, gemSpawnEntity, &AZ::TransformBus::Events::GetWorldTranslation);
            SpawnGem(position, type);
        }
    }

    void GemSpawnerComponentController::SpawnGem(const AZ::Vector3& location, const AZ::Crc32& type)
    {
        if (GetParent().GetGemSpawnables().empty())
        {
            return;
        }

        const GemSpawnable* spawnable = nullptr;
        for (const GemSpawnable& gemType : GetParent().GetGemSpawnables())
        {
            if (type == AZ::Crc32(gemType.m_tag.c_str()))
            {
                spawnable = &gemType;
            }
        }

        if (!spawnable)
        {
            return;
        }

        PrefabCallbacks callbacks;
        callbacks.m_onActivateCallback = [this](AZStd::shared_ptr<AzFramework::EntitySpawnTicket> ticket,
            AzFramework::SpawnableConstEntityContainerView view)
        {
            for (const AZ::Entity* entity : view)
            {
                if (GemComponent* gem = entity->FindComponent<GemComponent>())
                {
                    static_cast<GemComponentController*>(gem->GetController())->SetRandomPeriodOffset(
                        GetNetworkRandomComponentController()->GetRandomInt() % 1000);

                    // Save the gem spawn ticket, otherwise the gem will de-spawn
                    m_spawnedGems.emplace(entity->GetId(), AZStd::move(ticket));
                    break;
                }
            }
        };

        GetParent().GetNetworkPrefabSpawnerComponent()->SpawnPrefabAsset(
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), location),
            spawnable->m_gemAsset, AZStd::move(callbacks));
    }

    void GemSpawnerComponentController::RemoveGems()
    {
        for (const auto& gem : m_spawnedGems)
        {
            const auto netEntityId = Multiplayer::GetNetworkEntityManager()->GetNetEntityIdById(gem.first);
            auto netEntityHandle = Multiplayer::GetNetworkEntityManager()->GetEntity(netEntityId);

            if (netEntityHandle.Exists())
            {
                // Currently blocked by https://github.com/o3de/o3de/issues/11695
                /*AZ::EntityBus::MultiHandler::BusConnect(gem.first);
                Multiplayer::GetNetworkEntityManager()->MarkForRemoval(netEntityHandle);*/

                // Until https://github.com/o3de/o3de/issues/11695 is fixed, move the gem out of the view
                AZ::TransformBus::Event(gem.first, &AZ::TransformBus::Events::SetWorldTranslation, AZ::Vector3::CreateAxisZ(-1000.f));

                m_queuedForRemovalGems.emplace(gem.first, gem.second);
            }
        }

        m_spawnedGems.clear();
    }

    void GemSpawnerComponentController::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
        m_queuedForRemovalGems.erase(entityId);
    }

    AZ::Crc32 GemSpawnerComponentController::ChooseGemType(const LmbrCentral::Tags& tags)
    {
        if (GetSpawnTablesPerRound().empty())
        {
            return {};
        }

        const uint16_t round = GetNetworkMatchComponentController()->GetRoundNumber();
        const RoundSpawnTable* table = nullptr;
        if (round < GetSpawnTablesPerRound().size())
        {
            table = &GetSpawnTablesPerRound()[round];
        }
        else
        {
            table = &GetSpawnTablesPerRound().back();
        }

        float totalWeight = 0.f;
        for (const GemWeightChance& gemWeight : table->m_gemWeights)
        {
            const auto tagIterator = tags.find(AZ::Crc32(gemWeight.m_tag));
            if (tagIterator != tags.end())
            {
                totalWeight += gemWeight.m_weight;
            }
        }

        AZ::Crc32 chosenType(table->m_gemWeights.front().m_tag);
        float randomWeight = GetNetworkRandomComponentController()->GetRandomFloat() * totalWeight;
        for (const GemWeightChance& gemWeight : table->m_gemWeights)
        {
            const auto tagIterator = tags.find(AZ::Crc32(gemWeight.m_tag));
            if (tagIterator == tags.end())
            {
                continue;
            }

            if (randomWeight > gemWeight.m_weight)
            {
                randomWeight -= gemWeight.m_weight;
            }
            else
            {
                chosenType = AZ::Crc32(gemWeight.m_tag);
                break;
            }
        }

        return chosenType;
    }
}

#pragma optimize("", on)
