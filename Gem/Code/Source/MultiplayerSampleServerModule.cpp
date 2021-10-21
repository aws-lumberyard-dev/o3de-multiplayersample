/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "MultiplayerSampleServerSystemComponent.h"

namespace MultiplayerSample
{
    class MultiplayerSampleServerModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MultiplayerSampleServerModule, "{99CC4C0A-5897-4A97-A853-025AC2124097}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MultiplayerSampleServerModule, AZ::SystemAllocator, 0);

        MultiplayerSampleServerModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                MultiplayerSampleServerSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MultiplayerSampleServerSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_MultiplayerSample_Server, MultiplayerSample::MultiplayerSampleServerModule)
