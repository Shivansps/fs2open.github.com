# Arm Accuracy Super Resolution

FSO builds the Arm ASR 25.06 Vulkan backend as a static library. CMake selects the backend at build time. By default, x86-64 Windows and x86-64 Linux use AMD FidelityFX, and other targets use Arm ASR. This directory contains the C++ sources, headers and shaders from [Arm's generic library at revision `0501f490bd9946a2e1806b5363d7ab8a9a6a5e0a`](https://github.com/arm/accuracy-super-resolution-generic-library/tree/0501f490bd9946a2e1806b5363d7ab8a9a6a5e0a). The build uses the upstream SPIR-V headers. See [the MIT license](LICENSES/arm-asr.txt) for the copyright and license terms.

The [FSO integration patch](fso-integration.patch) records the changes applied to the Arm SDK. From the root of a checkout of that revision, run `git apply --ignore-space-change /path/to/fso-integration.patch`.

- Use FSO Vulkan instance and device functions with `VK_NO_PROTOTYPES`.
- Synchronize color attachment writes and resources shared by the fragment and compute stages.
- Preserve 64-bit Vulkan handles in 32-bit processes.
- Allow host-visible device-local memory as a fallback on unified-memory GPUs.
- Account for the resource-view table in scratch memory, check context alignment, and return an error for unknown shader passes.
- Use FP16 instead of FP8 for the HDR color input in the Performance profile.
- Use portable C++ headers and types for Windows and macOS builds.
