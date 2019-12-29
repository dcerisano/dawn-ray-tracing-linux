// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/vulkan/RayTracingPipelineVk.h"

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/PipelineLayoutVk.h"
#include "dawn_native/vulkan/ShaderModuleVk.h"
#include "dawn_native/vulkan/RayTracingShaderBindingTableVk.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    // static
    ResultOrError<RayTracingPipeline*> RayTracingPipeline::Create(
        Device* device,
        const RayTracingPipelineDescriptor* descriptor) {
        std::unique_ptr<RayTracingPipeline> pipeline =
            std::make_unique<RayTracingPipeline>(device, descriptor);
        DAWN_TRY(pipeline->Initialize(descriptor));
        return pipeline.release();
    }

    MaybeError RayTracingPipeline::Initialize(const RayTracingPipelineDescriptor* descriptor) {
        Device* device = ToBackend(GetDevice());

        RayTracingShaderBindingTable* shaderBindingTable =
            ToBackend(descriptor->rayTracingState->shaderBindingTable);

        std::vector<VkPipelineShaderStageCreateInfo> stages = shaderBindingTable->GetStages();
        std::vector<VkRayTracingShaderGroupCreateInfoNV> groups = shaderBindingTable->GetGroups();

        VkRayTracingPipelineCreateInfoNV createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
        createInfo.maxRecursionDepth = descriptor->rayTracingState->maxRecursionDepth;
        createInfo.stageCount = stages.size();
        createInfo.pStages = stages.data();
        createInfo.pGroups = groups.data();
        createInfo.groupCount = groups.size();
        createInfo.layout = ToBackend(descriptor->layout)->GetHandle();

        {
            MaybeError result = CheckVkSuccess(
                device->fn.CreateRayTracingPipelinesNV(device->GetVkDevice(), VK_NULL_HANDLE, 1,
                                                       &createInfo, nullptr, &mHandle),
                "CreateRayTracingPipelinesNV");
            if (result.IsError())
                return result.AcquireError();
        }

        DynamicUploader* uploader = device->GetDynamicUploader();
        uint64_t bufferSize = stages.size() * shaderBindingTable->GetShaderGroupHandleSize();
        DAWN_TRY_ASSIGN(mGroupBufferHandle,
                        uploader->Allocate(bufferSize, device->GetPendingCommandSerial()));

        {
            MaybeError result = CheckVkSuccess(device->fn.GetRayTracingShaderGroupHandlesNV(
                                                   device->GetVkDevice(), mHandle, 0, stages.size(),
                                                   bufferSize, mGroupBufferHandle.mappedBuffer),
                                               "GetRayTracingShaderGroupHandlesNV");
            if (result.IsError())
                return result.AcquireError();
        }

        return {};
    }

    RayTracingPipeline::~RayTracingPipeline() {
        if (mHandle != VK_NULL_HANDLE) {
            ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkPipeline RayTracingPipeline::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan