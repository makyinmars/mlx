// Copyright © 2023-2024 Apple Inc.

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include "python/src/convert.h"

#include "mlx/fast.h"
#include "mlx/ops.h"

#include <iostream>
#include <set>

namespace nb = nanobind;
using namespace nb::literals;
using namespace mlx::core;

void init_fast(nb::module_& parent_module) {
  auto m =
      parent_module.def_submodule("fast", "mlx.core.fast: fast operations");

  m.def(
      "rms_norm",
      &fast::rms_norm,
      "x"_a,
      "weight"_a,
      "eps"_a,
      nb::kw_only(),
      "stream"_a = nb::none(),
      nb::sig(
          "def rms_norm(x: array, weight: array, eps: float, *, stream: Union[None, Stream, Device] = None) -> array"),
      R"pbdoc(
        Root Mean Square normalization (RMS norm).

        The normalization is with respect to the last axis of the input ``x``.

        Args:
            x (array): Input array.
            weight (array): A multiplicative weight to scale the result by.
              The ``weight`` should be one-dimensional with the same size
              as the last axis of ``x``.
            eps (float): A small additive constant for numerical stability.

        Returns:
            array: The output array.
      )pbdoc");

  m.def(
      "layer_norm",
      &fast::layer_norm,
      "x"_a,
      "weight"_a.none(),
      "bias"_a.none(),
      "eps"_a,
      nb::kw_only(),
      "stream"_a = nb::none(),
      nb::sig(
          "def layer_norm(x: array, weight: Optional[array], bias: Optional[array], eps: float, *, stream: Union[None, Stream, Device] = None) -> array"),
      R"pbdoc(
        Layer normalization.

        The normalization is with respect to the last axis of the input ``x``.

        Args:
            x (array): Input array.
            weight (array, optional): A multiplicative weight to scale the result by.
              The ``weight`` should be one-dimensional with the same size
              as the last axis of ``x``. If set to ``None`` then no scaling happens.
            bias (array, optional): An additive offset to be added to the result.
              The ``bias`` should be one-dimensional with the same size
              as the last axis of ``x``. If set to ``None`` then no translation happens.
            eps (float): A small additive constant for numerical stability.

        Returns:
            array: The output array.
      )pbdoc");

  m.def(
      "rope",
      &fast::rope,
      "a"_a,
      "dims"_a,
      nb::kw_only(),
      "traditional"_a,
      "base"_a,
      "scale"_a,
      "offset"_a,
      "stream"_a = nb::none(),
      nb::sig(
          "def rope(a: array, dims: int, *, traditional: bool, base: float, scale: float, offset: int, stream: Union[None, Stream, Device] = None) -> array"),
      R"pbdoc(
        Apply rotary positional encoding to the input.

        Args:
            a (array): Input array.
            dims (int): The feature dimensions to be rotated. If the input feature
                is larger than dims then the rest is left unchanged.
            traditional (bool): If set to ``True`` choose the traditional
                implementation which rotates consecutive dimensions.
            base (float): The base used to compute angular frequency for
                each dimension in the positional encodings.
            scale (float): The scale used to scale the positions.
            offset (int): The position offset to start at.

        Returns:
            array: The output array.
      )pbdoc");

  m.def(
      "scaled_dot_product_attention",
      &fast::scaled_dot_product_attention,
      "q"_a,
      "k"_a,
      "v"_a,
      nb::kw_only(),
      "scale"_a,
      "mask"_a = nb::none(),
      "stream"_a = nb::none(),
      nb::sig(
          "def scaled_dot_product_attention(q: array, k: array, v: array, *, scale: float,  mask: Union[None, array] = None, stream: Union[None, Stream, Device] = None) -> array"),
      R"pbdoc(
        A fast implementation of multi-head attention: ``O = softmax(Q @ K.T, dim=-1) @ V``.

        Supports:

        * `Multi-Head Attention <https://arxiv.org/abs/1706.03762>`_
        * `Grouped Query Attention <https://arxiv.org/abs/2305.13245>`_
        * `Multi-Query Attention <https://arxiv.org/abs/1911.02150>`_

        Note: The softmax operation is performed in ``float32`` regardless of
        the input precision.

        Note: For Grouped Query Attention and Multi-Query Attention, the ``k``
        and ``v`` inputs should not be pre-tiled to match ``q``.

        Args:
            q (array): Input query array.
            k (array): Input keys array.
            v (array): Input values array.
            scale (float): Scale for queries (typically ``1.0 / sqrt(q.shape(-1)``)
            mask (array, optional): An additive mask to apply to the query-key scores.
        Returns:
            array: The output array.
      )pbdoc");

  nb::class_<fast::MetalKernel>(
      m,
      "MetalKernel",
      R"pbdoc(
      A custom Metal kernel defined from a source string.
      )pbdoc")
      .def(
          nb::init<
              const std::string&,
              const std::string&,
              std::map<std::string, std::vector<int>>,
              std::map<std::string, Dtype>,
              std::tuple<int, int, int>,
              std::tuple<int, int, int>,
              bool>(),
          "name"_a,
          "source"_a,
          "output_shapes"_a,
          "output_dtypes"_a,
          "grid"_a,
          "threadgroup"_a,
          "ensure_row_contiguous"_a = true)
      .def(
          "template",
          [](fast::MetalKernel& kernel, nb::kwargs kwargs) {
            kernel.template_args.clear();
            for (auto kv : kwargs) {
              auto name = nb::cast<std::string>(kv.first);
              auto value = kv.second;
              // Handle bool, int and dtype template args
              if (PyBool_Check(value.ptr())) {
                bool bool_val = nb::cast<bool>(value);
                kernel.template_args.insert({name, bool_val});
              } else if (PyLong_Check(value.ptr())) {
                int int_val = nb::cast<int>(value);
                kernel.template_args.insert({name, int_val});
              } else if (
                  nb::type_check(value.type()) &&
                  nb::type_info(value.type()) == typeid(Dtype)) {
                Dtype dtype = nb::cast<Dtype>(value);
                kernel.template_args.insert({name, dtype});
              } else {
                throw std::invalid_argument(
                    "Invalid template argument. Must be `mlx.core.Dtype`, `int` or `bool`.");
              }
            }
          })
      .def(
          "__call__",
          [](fast::MetalKernel& kernel, nb::kwargs& kwargs) {
            StreamOrDevice stream = {};
            if (kwargs.contains("stream")) {
              stream = nb::cast<StreamOrDevice>(kwargs["stream"]);
            }
            std::map<std::string, array> inputs;
            for (auto kv : kwargs) {
              auto name = nb::cast<std::string>(kv.first);
              if (name != "stream") {
                auto value = nb::cast<ArrayInitType>(kv.second);
                auto arr = create_array(value, std::nullopt);
                inputs.insert({name, arr});
              }
            }
            return kernel.run(inputs, stream);
          },
          nb::sig(
              "def __call__(self, **kwargs: array, stream: Union[None, Stream, Device] = None)"));
}
