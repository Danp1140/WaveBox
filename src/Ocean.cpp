#include "Ocean.h"

PipelineInfo Ocean::graphicspipeline = {};
PipelineInfo Ocean::computepipeline = {};
VkSampler Ocean::heightmapsampler = VK_NULL_HANDLE;
GH* Ocean::gh = nullptr;

Ocean::Ocean(GH* g) {
	gh = g;
	if (graphicspipeline.pipeline == VK_NULL_HANDLE) initGraphicsPipeline();
	if (computepipeline.pipeline == VK_NULL_HANDLE) initComputePipeline();

	LinearWaveData ltemp = LinearWaveData(1.0, 10.0, 100.0);
	waves.emplace_back(ltemp, DEPTH_TYPE_CONSTANT);
	
	initBuffers();
	heightmap.extent = {HEIGHT_MAP_RESOLUTION, HEIGHT_MAP_RESOLUTION};
	heightmap.format = VK_FORMAT_R8_SNORM;
	heightmap.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	heightmap.layout = VK_IMAGE_LAYOUT_GENERAL;
	gh->createImage(heightmap);
	initDescriptorSets();
}

Ocean::~Ocean() {
	terminateDescriptorSets();
	gh->destroyImage(heightmap);
	terminateBuffers();
}

void Ocean::initGraphicsPipeline() {
	graphicspipeline.stages = VK_SHADER_STAGE_VERTEX_BIT
		| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT;
	graphicspipeline.topo = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	graphicspipeline.shaderfilepathprefix = "ocean";
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[2] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, 
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
	};
	graphicspipeline.vertexinputstateci = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, 0,
		1, &vertinbindingdesc,
		2, &vertinattribdesc[0]
	};
	graphicspipeline.pushconstantrange = {
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		sizeof(OceanGraphicsPCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[1] {{
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}};
	graphicspipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&dslbindings[0]
	};

	gh->createPipeline(graphicspipeline);

	gh->createSampler(heightmapsampler, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void Ocean::initComputePipeline() {
	computepipeline.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computepipeline.shaderfilepathprefix = "ocean";
	computepipeline.pushconstantrange = {
		VK_SHADER_STAGE_COMPUTE_BIT,
		0u,
		sizeof(OceanComputePCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[2] {{
		0,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}};
	computepipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		2, &dslbindings[0]
	};

	gh->createPipeline(computepipeline);
}

void Ocean::terminatePipelines() {
	gh->destroySampler(heightmapsampler);

	if (graphicspipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(graphicspipeline);
	}
	if (computepipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(computepipeline);
	}
}

void Ocean::recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline.pipeline);
	vkCmdPushConstants(
		cb,
		graphicspipeline.layout, 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
		0u,
		sizeof(OceanGraphicsPCData),
		data.pcdata);
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		graphicspipeline.layout,
		0u, 1u, &data.ds,
		0u, nullptr);
	VkDeviceSize dummyoffset = 0u;
	vkCmdBindVertexBuffers(cb, 0, 1, &data.pVertexBuffer->buffer, &dummyoffset);
	vkCmdBindIndexBuffer(cb, data.pIndexBuffer->buffer, 0u, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(cb, data.pIndexBuffer->size / sizeof(Index), 1u, 0u, 0u, 0u);
}

void Ocean::recordComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, computepipeline.pipeline);
	vkCmdPushConstants(
		cb,
		computepipeline.layout, 
		VK_SHADER_STAGE_COMPUTE_BIT, 
		0u,
		sizeof(OceanComputePCData),
		data.pcdata);
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		computepipeline.layout,
		0u, 1u, &data.ds,
		0u, nullptr);
	vkCmdDispatch(cb, HEIGHT_MAP_RESOLUTION, HEIGHT_MAP_RESOLUTION, 1);
}

void Ocean::initBuffers() {
	std::vector<Vertex> verttemp;
	verttemp.push_back({glm::vec3(-1, 0, -1), glm::vec2(0, 0)});
	verttemp.push_back({glm::vec3(-1, 0, 1), glm::vec2(0, 1)});
	verttemp.push_back({glm::vec3(1, 0, -1), glm::vec2(1, 0)});
	verttemp.push_back({glm::vec3(1, 0, 1), glm::vec2(1, 1)});

	for (auto& v : verttemp) {
		v.position *= 10;
	}

	std::vector<Index> idxtemp;
	idxtemp.push_back(0);
	idxtemp.push_back(1);
	idxtemp.push_back(2);
	idxtemp.push_back(1);
	idxtemp.push_back(3);
	idxtemp.push_back(2);

	gh->createVertexAndIndexBuffers(vertexbuffer, indexbuffer, verttemp, idxtemp);
	
	wavebuffer.size = waves.size() * sizeof(Wave);
	wavebuffer.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	wavebuffer.memprops = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	gh->createBuffer(wavebuffer);

	gh->updateHostCoherentBuffer(wavebuffer, reinterpret_cast<void *>(waves.data()));


}

void Ocean::terminateBuffers() {
	gh->destroyBuffer(wavebuffer);
	gh->destroyBuffer(vertexbuffer);
	gh->destroyBuffer(indexbuffer);
}

void Ocean::initDescriptorSets() {
	VkDescriptorImageInfo di {heightmapsampler, heightmap.view, heightmap.layout};
	gh->createDescriptorSet(
		graphicsdescriptorset, 
		graphicspipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		&di, nullptr);
	VkDescriptorImageInfo ii[2] = {di, {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED}};
	VkDescriptorBufferInfo bi[2] = {{VK_NULL_HANDLE, 0u, 0u}, {wavebuffer.buffer, 0u, VK_WHOLE_SIZE}};
	gh->createDescriptorSet(
		computedescriptorset, 
		computepipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
		&ii[0], &bi[0]);
}

void Ocean::terminateDescriptorSets() {
	gh->destroyDescriptorSet(graphicsdescriptorset);
	gh->destroyDescriptorSet(computedescriptorset);
}
