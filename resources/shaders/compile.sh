path="/Users/danp/Desktop/C Coding/wavebox/resources/shaders"
cd "$path"

echo "Compiling Ocean Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/OceanVertex.glsl -o SPIRV/oceanvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/OceanTessellationControl.glsl -o SPIRV/oceantesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/OceanTessellationEvaluation.glsl -o SPIRV/oceantese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/OceanFragment.glsl -o SPIRV/oceanfrag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/OceanCompute.glsl -o SPIRV/oceancomp.spv

#/Users/danp/downloads/install/bin/spirv-dis "/Users/danp/Desktop/C Coding/VulkanSandbox/resources/shaders/SPIRV/voxeltroubleshootingvert.spv"

#od -x SPIRV/voxeltroubleshootingvert.spv

