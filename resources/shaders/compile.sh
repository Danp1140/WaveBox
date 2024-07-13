path="/Users/danp/Desktop/C Coding/wavebox/resources/shaders"
cd "$path"

echo "Compiling Ocean Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/OceanVertex.glsl -o SPIRV/oceanvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/OceanTessellationControl.glsl -o SPIRV/oceantesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/OceanTessellationEvaluation.glsl -o SPIRV/oceantese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/OceanFragment.glsl -o SPIRV/oceanfrag.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/OceanCompute.glsl -o SPIRV/oceancomp.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=comp GLSL/OceanPropertyCompute.glsl -o SPIRV/oceanpropcomp.spv

echo "Compiling Mesh Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/MeshVertex.glsl -o SPIRV/meshvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tesc GLSL/MeshTessellationControl.glsl -o SPIRV/meshtesc.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=tese GLSL/MeshTessellationEvaluation.glsl -o SPIRV/meshtese.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/MeshFragment.glsl -o SPIRV/meshfrag.spv

echo "Compiling Depth Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/DepthVertex.glsl -o SPIRV/depthvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/DepthFragment.glsl -o SPIRV/depthfrag.spv

echo "Compiling Env Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/EnvVertex.glsl -o SPIRV/envvert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/EnvFragment.glsl -o SPIRV/envfrag.spv

echo "Compiling UI Shaders!"
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=vert GLSL/UIVertex.glsl -o SPIRV/uivert.spv
/Users/danp/VulkanSDK/1.2.176.1/macOS/bin/glslc -fshader-stage=frag GLSL/UIFragment.glsl -o SPIRV/uifrag.spv

