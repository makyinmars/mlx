// Copyright © 2023-2024 Apple Inc.

#pragma once

#include <any>
#include <map>
#include <optional>

#include "mlx/utils.h"

namespace mlx::core::fast {

array rms_norm(
    const array& x,
    const array& weight,
    float eps,
    StreamOrDevice s = {});

array layer_norm(
    const array& x,
    const std::optional<array>& weight,
    const std::optional<array>& bias,
    float eps,
    StreamOrDevice s = {});

array rope(
    const array& x,
    int dims,
    bool traditional,
    float base,
    float scale,
    int offset,
    StreamOrDevice s = {});

/** Computes: O = softmax(Q @ K.T) @ V **/
array scaled_dot_product_attention(
    const array& queries,
    const array& keys,
    const array& values,
    const float scale,
    const std::optional<array>& mask = std::nullopt,
    StreamOrDevice s = {});

std::tuple<array, array, array> affine_quantize(
    const array& w,
    int group_size = 64,
    int bits = 4,
    StreamOrDevice s = {});

array affine_quantize(
    const array& w,
    const array& scales,
    const array& biases,
    int group_size = 64,
    int bits = 4,
    StreamOrDevice s = {});

array affine_dequantize(
    const array& w,
    const array& scales,
    const array& biases,
    int group_size = 64,
    int bits = 4,
    StreamOrDevice s = {});

std::vector<array> custom_kernel(
    std::string name,
    std::map<std::string, std::any>& inputs,
    std::vector<std::string>& template_args,
    const std::string& source,
    std::map<std::string, std::vector<int>> output_shapes,
    std::map<std::string, Dtype> output_dtypes,
    std::tuple<int, int, int> grid,
    std::tuple<int, int, int> threadgroup,
    bool ensure_row_contiguous = true,
    StreamOrDevice s = {});

} // namespace mlx::core::fast
