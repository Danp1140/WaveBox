#include "Drawable.h"

GH* Drawable::gh = nullptr;
PipelineInfo Drawable::DTHgraphicspipeline = {};
VkSampler Drawable::heightmapsampler = VK_NULL_HANDLE;

Drawable::Drawable(GH* g) {
	gh = g;
	if (heightmapsampler == VK_NULL_HANDLE) initSamplers();
}

Drawable::~Drawable() {
	terminateBuffers();
	terminateDescriptorSets();
}

void Drawable::initSamplers() {
	gh->createSampler(heightmapsampler, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void Drawable::terminateSamplers() {
	gh->destroySampler(heightmapsampler);
}

/*
void Drawable::initPipelines() {
	DTHgraphicspipeline.stages = VK_SHADER_STAGE_VERTEX_BIT
		| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT;
	DTHgraphicspipeline.topo = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	DTHgraphicspipeline.shaderfilepathprefix = "dth"; 
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[2] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, 
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
	};
	DTHgraphicspipeline.vertexinputstateci = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, 0,
		1, &vertinbindingdesc,
		2, &vertinattribdesc[0]
	};
	DTHgraphicspipeline.pushconstantrange = {
		VK_SHADER_STAGE_VERTEX_BIT 
			| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT 
			| VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		sizeof(DTHGraphicsPCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[2] {{
		0, // heightmap
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		1, // diffuse text
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	DTHgraphicspipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		2,
		&dslbindings[0]
	};
	DTHgraphicspipeline.depthtest = true;

	gh->createPipeline(DTHgraphicspipeline);

	gh->createSampler(heightmapsampler, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}
*/

/*
void Drawable::terminatePipelines() {
	gh->destroySampler(heightmapsampler);
	if (DTHgraphicspipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(DTHgraphicspipeline);
	}
}
*/
