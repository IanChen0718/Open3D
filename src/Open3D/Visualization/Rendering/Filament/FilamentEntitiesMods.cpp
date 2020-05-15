// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2019 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "Open3D/Visualization/Rendering/Filament/FilamentEntitiesMods.h"

#include <filament/MaterialInstance.h>
#include <filament/TextureSampler.h>

#include "Open3D/Utility/Console.h"
#include "Open3D/Visualization/Rendering/Filament/FilamentEngine.h"
#include "Open3D/Visualization/Rendering/Filament/FilamentResourceManager.h"

namespace open3d {
namespace visualization {

namespace {

using namespace filament;

TextureSampler::WrapMode ConvertWrapMode(
        TextureSamplerParameters::WrapMode mode) {
    switch (mode) {
        case TextureSamplerParameters::WrapMode::ClampToEdge:
            return TextureSampler::WrapMode::CLAMP_TO_EDGE;
        case TextureSamplerParameters::WrapMode::Repeat:
            return TextureSampler::WrapMode::REPEAT;
        case TextureSamplerParameters::WrapMode::MirroredRepeat:
            return TextureSampler::WrapMode::MIRRORED_REPEAT;
    }

    return TextureSampler::WrapMode::CLAMP_TO_EDGE;
}

}  // namespace

TextureSampler FilamentMaterialModifier::SamplerFromSamplerParameters(
        const TextureSamplerParameters& sampler_config) {
    TextureSampler sampler;

    switch (sampler_config.filter_mag) {
        case TextureSamplerParameters::MagFilter::Nearest:
            sampler.setMagFilter(TextureSampler::MagFilter::NEAREST);
            break;
        case TextureSamplerParameters::MagFilter::Linear:
            sampler.setMagFilter(TextureSampler::MagFilter::LINEAR);
            break;
    }

    switch (sampler_config.filter_min) {
        case TextureSamplerParameters::MinFilter::Nearest:
            sampler.setMinFilter(TextureSampler::MinFilter::NEAREST);
            break;
        case TextureSamplerParameters::MinFilter::Linear:
            sampler.setMinFilter(TextureSampler::MinFilter::LINEAR);
            break;
        case TextureSamplerParameters::MinFilter::NearestMipmapNearest:
            sampler.setMinFilter(
                    TextureSampler::MinFilter::NEAREST_MIPMAP_NEAREST);
            break;
        case TextureSamplerParameters::MinFilter::LinearMipmapNearest:
            sampler.setMinFilter(
                    TextureSampler::MinFilter::LINEAR_MIPMAP_NEAREST);
            break;
        case TextureSamplerParameters::MinFilter::NearestMipmapLinear:
            sampler.setMinFilter(
                    TextureSampler::MinFilter::NEAREST_MIPMAP_LINEAR);
            break;
        case TextureSamplerParameters::MinFilter::LinearMipmapLinear:
            sampler.setMinFilter(
                    TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR);
            break;
    }

    sampler.setWrapModeS(ConvertWrapMode(sampler_config.wrap_u));
    sampler.setWrapModeT(ConvertWrapMode(sampler_config.wrap_v));
    sampler.setWrapModeR(ConvertWrapMode(sampler_config.wrap_w));

    sampler.setAnisotropy(sampler.getAnisotropy());

    return sampler;
}

FilamentMaterialModifier::FilamentMaterialModifier(
        const std::shared_ptr<filament::MaterialInstance>& material_instance,
        const MaterialInstanceHandle& id) {
    Init(material_instance, id);
}

void FilamentMaterialModifier::Reset() {
    if (material_instance_ != nullptr) {
        utility::LogWarning(
                "Previous material instance modifications are not finished!");
    }

    material_instance_ = nullptr;
    current_handle_ = MaterialInstanceHandle::kBad;
}

void FilamentMaterialModifier::Init(
        const std::shared_ptr<filament::MaterialInstance>& material_instance,
        const MaterialInstanceHandle& id) {
    if (material_instance_ != nullptr) {
        utility::LogWarning(
                "Previous material instance modifications are not finished!");
    }

    material_instance_ = material_instance;
    current_handle_ = id;
}

MaterialModifier& FilamentMaterialModifier::SetParameter(const char* parameter,
                                                         const int value) {
    if (material_instance_) {
        material_instance_->setParameter(parameter, value);
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetParameter(const char* parameter,
                                                         const float value) {
    if (material_instance_) {
        material_instance_->setParameter(parameter, value);
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetParameter(
        const char* parameter, const Eigen::Vector3f& v) {
    if (material_instance_) {
        material_instance_->setParameter(parameter,
                                         math::float3{v(0), v(1), v(2)});
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetColor(
        const char* parameter, const Eigen::Vector3f& value) {
    if (material_instance_) {
        const auto color =
                filament::math::float3{value.x(), value.y(), value.z()};
        material_instance_->setParameter(parameter, filament::RgbType::LINEAR,
                                         color);
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetColor(
        const char* parameter, const Eigen::Vector4f& value) {
    if (material_instance_) {
        auto color =
                filament::math::float4{value(0), value(1), value(2), value(3)};
        material_instance_->setParameter(parameter, filament::RgbaType::LINEAR,
                                         color);
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetTexture(
        const char* parameter,
        const TextureHandle& texture_handle,
        const TextureSamplerParameters& sampler_config) {
    if (material_instance_) {
        auto w_texture =
                EngineInstance::GetResourceManager().GetTexture(texture_handle);

        if (auto texture_ptr = w_texture.lock()) {
            material_instance_->setParameter(
                    parameter, texture_ptr.get(),
                    SamplerFromSamplerParameters(sampler_config));
        } else {
            utility::LogWarning(
                    "Failed to set texture for material.\n\tMaterial handle: "
                    "{}\n\tTexture handle: {}\n\tParameter name: {}",
                    current_handle_, texture_handle, parameter);
        }
    }

    return *this;
}

MaterialModifier& FilamentMaterialModifier::SetDoubleSided(bool double_sided) {
    if (material_instance_) {
        material_instance_->setDoubleSided(double_sided);
    }
    return *this;
}

MaterialInstanceHandle FilamentMaterialModifier::Finish() {
    auto res = current_handle_;

    material_instance_ = nullptr;
    current_handle_ = MaterialInstanceHandle::kBad;

    return res;
}

}  // namespace visualization
}  // namespace open3d
