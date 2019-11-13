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

#include "dawn_native/ComputePassEncoder.h"

#include "dawn_native/Buffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/Device.h"

namespace dawn_native {

    ComputePassEncoder::ComputePassEncoder(DeviceBase* device,
                                           CommandEncoder* commandEncoder,
                                           EncodingContext* encodingContext)
        : ProgrammablePassEncoder(device, encodingContext), mCommandEncoder(commandEncoder) {
    }

    ComputePassEncoder::ComputePassEncoder(DeviceBase* device,
                                           CommandEncoder* commandEncoder,
                                           EncodingContext* encodingContext,
                                           ErrorTag errorTag)
        : ProgrammablePassEncoder(device, encodingContext, errorTag),
          mCommandEncoder(commandEncoder) {
    }

    ComputePassEncoder* ComputePassEncoder::MakeError(DeviceBase* device,
                                                      CommandEncoder* commandEncoder,
                                                      EncodingContext* encodingContext) {
        return new ComputePassEncoder(device, commandEncoder, encodingContext, ObjectBase::kError);
    }

    void ComputePassEncoder::EndPass() {
        if (mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
                allocator->Allocate<EndComputePassCmd>(Command::EndComputePass);

                return {};
            })) {
            mEncodingContext->ExitPass(this);
        }
    }

    void ComputePassEncoder::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DispatchCmd* dispatch = allocator->Allocate<DispatchCmd>(Command::Dispatch);
            dispatch->x = x;
            dispatch->y = y;
            dispatch->z = z;

            return {};
        });
    }

    void ComputePassEncoder::DispatchIndirect(BufferBase* indirectBuffer, uint64_t indirectOffset) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(indirectBuffer));

            if (indirectOffset >= indirectBuffer->GetSize() ||
                indirectOffset + kDispatchIndirectSize > indirectBuffer->GetSize()) {
                return DAWN_VALIDATION_ERROR("Indirect offset out of bounds");
            }

            DispatchIndirectCmd* dispatch =
                allocator->Allocate<DispatchIndirectCmd>(Command::DispatchIndirect);
            dispatch->indirectBuffer = indirectBuffer;
            dispatch->indirectOffset = indirectOffset;

            return {};
        });
    }

    void ComputePassEncoder::SetPipeline(ComputePipelineBase* pipeline) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(pipeline));

            SetComputePipelineCmd* cmd =
                allocator->Allocate<SetComputePipelineCmd>(Command::SetComputePipeline);
            cmd->pipeline = pipeline;

            return {};
        });
    }

}  // namespace dawn_native
