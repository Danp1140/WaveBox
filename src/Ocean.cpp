#include "Ocean.h"

PipelineInfo Ocean::graphicspipeline = {};
PipelineInfo Ocean::computepipeline = {};
PipelineInfo Ocean::propertycomputepipeline = {};
PipelineInfo Ocean::shoalingcomputepipeline = {};

Ocean::Ocean(GH* g) : Drawable(g) {
	if (computepipeline.pipeline == VK_NULL_HANDLE) initComputePipeline();
	if (graphicspipeline.pipeline == VK_NULL_HANDLE) initGraphicsPipeline();

	presubdivision = 5;
	sidewalls = false;
	scale = 100.;
	floor = new Mesh(); // bodgey workaround to have dummy AABB for framebuffer creation, TODO: refine later

	LinearWaveData ltemp = LinearWaveData(2.0, 30.0, 100.0, glm::vec2(1., 1.));
	waves.emplace_back(ltemp, DEPTH_TYPE_CONSTANT);
	//ltemp = LinearWaveData(1.0, 7.0, 5.0, glm::vec2(1., 0.));
	//ltemp = LinearWaveData(1.0, 10.0, 100.0, glm::vec2(1., 0.));
	//waves.emplace_back(ltemp, DEPTH_TYPE_CONSTANT);
	// waves = piersonMoskowitzSample(10);
	
	initRenderpass();
	initFramebuffer();
	initDepthPipeline();
	initBuffers();
	heightmap.extent = {HEIGHT_MAP_RESOLUTION, HEIGHT_MAP_RESOLUTION};
	heightmap.format = VK_FORMAT_R32_SFLOAT;
	heightmap.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	heightmap.layout = VK_IMAGE_LAYOUT_GENERAL;
	gh->createImage(heightmap);

	displacementmap.extent = {DISPLACEMENT_MAP_RESOLUTION, DISPLACEMENT_MAP_RESOLUTION};
	// had to add A16 since rgba16 not supported on my device
	// TODO: add more dynamic format determining code
	displacementmap.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	displacementmap.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	displacementmap.layout = VK_IMAGE_LAYOUT_GENERAL;
	gh->createImage(displacementmap);

	// renderDepthMap();
	delete floor;
	floor = new Mesh(&depthmap);
	generateDepthMap();

	waves.back().linear.addkMap(g, depthmap);
	
	initDescriptorSets();
}

Ocean::~Ocean() {
	gh->destroyImage(waves.back().linear.kmap); // again, quick and dirty for quick proof of concept
	gh->destroyPipeline(depthpipeline);
	terminateFramebuffer();
	terminateRenderpass();
	if (floor) delete floor;
	gh->destroyImage(displacementmap);
	gh->destroyImage(heightmap);
	// TODO: figure out why the below overrides aren't being called by Drawable's destructor
	terminateBuffers();
	terminateDescriptorSets();
}

void Ocean::initGraphicsPipeline() {
	graphicspipeline.stages = VK_SHADER_STAGE_VERTEX_BIT
		| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
		| VK_SHADER_STAGE_FRAGMENT_BIT;
	graphicspipeline.topo = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	graphicspipeline.shaderfilepathprefix = "ocean"; 
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[3] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, 
		{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm)}
	};
	graphicspipeline.vertexinputstateci = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, 0,
		1, &vertinbindingdesc,
		3, &vertinattribdesc[0]
	};
	graphicspipeline.pushconstantrange = {
		VK_SHADER_STAGE_VERTEX_BIT 
			| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT 
			| VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		sizeof(OceanGraphicsPCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[4] {{
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
	}, {
		2, // env map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	}, {
		3, // disp map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		nullptr
	}};
	graphicspipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		4,
		&dslbindings[0]
	};
	graphicspipeline.depthtest = true;

	gh->createPipeline(graphicspipeline);
}

void Ocean::initComputePipeline() {
	computepipeline.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computepipeline.shaderfilepathprefix = "ocean";
	computepipeline.pushconstantrange = {
		VK_SHADER_STAGE_COMPUTE_BIT,
		0u,
		sizeof(OceanComputePCData)
	};
	VkDescriptorSetLayoutBinding dslbindings[5] {{
		0, // heightmap
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		1, // wave info buffer
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		2, // depth map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		3, // k map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1, // this should really be whatever number we're capping shoaling waves at
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		4, // disp map
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}};
	computepipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		5, &dslbindings[0]
	};

	gh->createPipeline(computepipeline);

	propertycomputepipeline.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	propertycomputepipeline.shaderfilepathprefix = "oceanprop";
	VkDescriptorSetLayoutBinding pdslbindings[3] {{
		0, // wave info buffer
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		1, // depth map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		2, // wave number map
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}};
	propertycomputepipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		3, &pdslbindings[0]
	};

	gh->createPipeline(propertycomputepipeline);

	shoalingcomputepipeline.stages = VK_SHADER_STAGE_COMPUTE_BIT;
	shoalingcomputepipeline.shaderfilepathprefix = "oceanshoal";
	shoalingcomputepipeline.pushconstantrange = {
		VK_SHADER_STAGE_COMPUTE_BIT,
		0u,
		sizeof(OceanShoalingComputePCData)
	};
	VkDescriptorSetLayoutBinding sdslbindings[3] {{
		0, // wave info buffer
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		1, // displacement map
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}, {
		2, // wave number map
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	}};
	shoalingcomputepipeline.descsetlayoutci = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		3, &sdslbindings[0]
	};

	gh->createPipeline(shoalingcomputepipeline);
}

void Ocean::initDepthPipeline() {
	depthpipeline.renderpass = depthrenderpass;
	depthpipeline.stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	depthpipeline.topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	depthpipeline.shaderfilepathprefix = "depth";
	VkVertexInputBindingDescription vertinbindingdesc {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription vertinattribdesc[1] {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}};
	depthpipeline.vertexinputstateci = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, 0,
		1, &vertinbindingdesc,
		1, &vertinattribdesc[0]
	};
	depthpipeline.pushconstantrange = {
		VK_SHADER_STAGE_VERTEX_BIT,
		0u,
		sizeof(DepthPCData)
	};
	depthpipeline.depthtest = true;

	gh->createPipeline(depthpipeline);
}

void Ocean::terminatePipelines() {
	if (graphicspipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(graphicspipeline);
	}
	if (computepipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(computepipeline);
	}
	if (propertycomputepipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(propertycomputepipeline);
	}
	if (shoalingcomputepipeline.pipeline != VK_NULL_HANDLE) {
		gh->destroyPipeline(shoalingcomputepipeline);
	}
}

void Ocean::recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline.pipeline);
	vkCmdPushConstants(
		cb,
		graphicspipeline.layout, 
		VK_SHADER_STAGE_VERTEX_BIT 
		| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT 
		| VK_SHADER_STAGE_FRAGMENT_BIT, 
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

void Ocean::recordPropertyComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, propertycomputepipeline.pipeline);
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		propertycomputepipeline.layout,
		0u, 1u, &data.ds,
		0u, nullptr);
	vkCmdDispatch(cb, 128, 128, 1);
}

void Ocean::recordShoalingComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data) {
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, shoalingcomputepipeline.pipeline);
	vkCmdPushConstants(
		cb,
		shoalingcomputepipeline.layout, 
		VK_SHADER_STAGE_COMPUTE_BIT, 
		0u,
		sizeof(OceanShoalingComputePCData),
		data.pcdata);
	vkCmdBindDescriptorSets(
		cb,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		shoalingcomputepipeline.layout,
		0u, 1u, &data.ds,
		0u, nullptr);
	vkCmdDispatch(cb, DISPLACEMENT_MAP_RESOLUTION, DISPLACEMENT_MAP_RESOLUTION, 1);
}

void Ocean::initRenderpass() {
	VkAttachmentDescription depthattachmentdescriptions[1] {{
		0,
		VK_FORMAT_D32_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
	}};
	VkAttachmentReference depthattachmentreferences[1] {{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}};
	VkSubpassDescription depthsubpassdescription {
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0, nullptr,
		0, nullptr, nullptr, &depthattachmentreferences[0],
		0, nullptr
	};
	VkSubpassDependency subpassdependency {
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		0
	};
	VkRenderPassCreateInfo depthrenderpasscreateinfo {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		1, &depthattachmentdescriptions[0],
		1, &depthsubpassdescription,
		1, &subpassdependency
	};
	vkCreateRenderPass(gh->logicaldevice, &depthrenderpasscreateinfo, nullptr, &depthrenderpass);
}

void Ocean::terminateRenderpass() {
	vkDestroyRenderPass(gh->logicaldevice, depthrenderpass, nullptr);
}

void Ocean::initFramebuffer() {
	uint32_t res = std::floor(log2f(glm::distance(floor->AABB[1], floor->AABB[0]))) * DEPTH_MAP_RESOLUTION;
	depthmap.extent = {res, res};
	depthmap.format = VK_FORMAT_D32_SFLOAT;
	depthmap.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	depthmap.layout = VK_IMAGE_LAYOUT_GENERAL;
	gh->createImage(depthmap);
	VkFramebufferCreateInfo framebufferci {
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		nullptr,
		0,
		depthrenderpass,
		1, &depthmap.view, 
		res, res, 1
	};
	vkCreateFramebuffer(gh->logicaldevice, &framebufferci, nullptr, &depthframebuffer);
}

void Ocean::terminateFramebuffer() {
	vkDestroyFramebuffer(gh->logicaldevice, depthframebuffer, nullptr);
	gh->destroyImage(depthmap);
}

void Ocean::initBuffers() {
	std::vector<Vertex> verttemp;
	verttemp.push_back({glm::vec3(-1, 0, -1), glm::vec2(0, 0)});
	verttemp.push_back({glm::vec3(-1, 0, 1), glm::vec2(0, 1)});
	verttemp.push_back({glm::vec3(1, 0, -1), glm::vec2(1, 0)});
	verttemp.push_back({glm::vec3(1, 0, 1), glm::vec2(1, 1)});

	for (auto& v : verttemp) {
		v.position *= scale;
	}

	std::vector<Index> idxtemp;
	idxtemp.push_back(0);
	idxtemp.push_back(1);
	idxtemp.push_back(2);
	idxtemp.push_back(1);
	idxtemp.push_back(3);
	idxtemp.push_back(2);

	generateMesh(verttemp, idxtemp);

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
	VkDescriptorImageInfo di[4] {
		{heightmapsampler, heightmap.view, heightmap.layout},
		{heightmapsampler, waves.back().linear.kmap.view, waves.back().linear.kmap.layout},
		{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
		{heightmapsampler, displacementmap.view, displacementmap.layout}
	};
	gh->createDescriptorSet(
		graphicsdescriptorset, 
		graphicspipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		&di[0], nullptr);

	VkDescriptorImageInfo ii[5] = {
		{heightmapsampler, heightmap.view, heightmap.layout}, 
		{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
		{heightmapsampler, depthmap.view, depthmap.layout},
		{heightmapsampler, waves.back().linear.kmap.view, waves.back().linear.kmap.layout}, // quick & dirty to test tech, TODO: make more functional later!
		{heightmapsampler, displacementmap.view, displacementmap.layout}
	};
	VkDescriptorBufferInfo bi[5] = {
		{VK_NULL_HANDLE, 0u, 0u}, 
		{wavebuffer.buffer, 0u, VK_WHOLE_SIZE},
		{VK_NULL_HANDLE, 0u, 0u},
		{VK_NULL_HANDLE, 0u, 0u},
		{VK_NULL_HANDLE, 0u, 0u}
	};
	gh->createDescriptorSet(
		computedescriptorset, 
		computepipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
		&ii[0], &bi[0]);

	VkDescriptorImageInfo pii[3] = {
		{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
		{heightmapsampler, depthmap.view, depthmap.layout},
		{heightmapsampler, waves.back().linear.kmap.view, waves.back().linear.kmap.layout} // quick & dirty to test tech, TODO: make more functional later!
	};
	VkDescriptorBufferInfo pbi[3] = {
		{wavebuffer.buffer, 0u, VK_WHOLE_SIZE},
		{VK_NULL_HANDLE, 0u, 0u},
		{VK_NULL_HANDLE, 0u, 0u}
	};

	gh->createDescriptorSet(
		propertycomputedescriptorset, 
		propertycomputepipeline.dsl, 
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
		&pii[0], &pbi[0]);

	VkDescriptorImageInfo sii[3] {
		{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
		{heightmapsampler, displacementmap.view, displacementmap.layout},
		{heightmapsampler, waves.back().linear.kmap.view, waves.back().linear.kmap.layout} 
	};
	VkDescriptorBufferInfo sbi[3] = {
		{wavebuffer.buffer, 0u, VK_WHOLE_SIZE},
		{VK_NULL_HANDLE, 0u, 0u},
		{VK_NULL_HANDLE, 0u, 0u}
	};
	gh->createDescriptorSet(
		shoalingcomputedescriptorset,
		shoalingcomputepipeline.dsl,
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
		&sii[0], &sbi[0]);
}

void Ocean::terminateDescriptorSets() {
	gh->destroyDescriptorSet(graphicsdescriptorset);
	gh->destroyDescriptorSet(computedescriptorset);
	gh->destroyDescriptorSet(propertycomputedescriptorset);
	gh->destroyDescriptorSet(shoalingcomputedescriptorset);
}

std::vector<Wave> Ocean::piersonMoskowitzSample(uint8_t n) {
	std::vector<Wave> result;
	LinearWaveData lwtemp = LinearWaveData(0., 0., 0., glm::vec2(0.));
	float f;
	for (uint8_t i = 0; i < n; i++) {
		f = ((float)i + 1.) * 0.01;
		std::cout << f << std::endl;
		lwtemp = LinearWaveData(
			1. * piersonMoskowitz(f, 15.0), 
			6.28 / inverseDispersionRelation(6.28 * f, 100.), 
			100.0,
			glm::normalize(glm::vec2(0., 0.) + 1. * glm::vec2(float(rand())/float(RAND_MAX), float(rand())/float(RAND_MAX))));
		result.emplace_back(lwtemp, DEPTH_TYPE_CONSTANT);
	}
	return result;
}

float Ocean::piersonMoskowitz(float f, float U10) {
	float beta = 0.67 * pow(9.8 / 6.28 / U10, 4);
	return 0.0081 * 96 * exp(-beta / pow(f, 4)) / (1558.5 * pow(f, 5));
}

float Ocean::inverseDispersionRelation(float omega, float d) {
	// couldn't find an analytic solution so have to go numeric
	
	/*
	 * lower bound is extremely thin, and so not worth using for now
	if (pow(omega, 2) / 9.8 < 0.3 / d) {
		std::cout << "low\n";
		return omega / sqrt(9.8 / d);
	}
	*/
	/*
	if (pow(omega, 2) / 9.8 > 3.0 / d) {
		return pow(omega, 2) / 9.8;
	}
	float minerr = 999., minerrk, temp;
	for (float k = 0.0; k < 3.0 / d; k += 0.001) {
		temp = abs(pow(omega, 2) - 9.8 * k * tanh(k * d));
		if (temp < minerr) {
			minerr = temp;
			minerrk = k;
		}
	}
	return minerrk;
	*/
	// below is RK4
	// TODO: add deep & shallow water approx
	// could be made more efficient w/ opt init conditions
	const float stepsize = 0.1;
	float k1, k2, k3, k4, kn = 0.01;
	auto dkdo = [](float df, float kf) {
		return (2 * sqrt(9.8 * kf * tanh(kf * df)))
			/ (9.8 * (tanh(kf * df) + kf * df * pow(1 / cosh(kf * df), 2)));
	};
	for (float o = sqrt(9.8 * kn * tanh(kn * d)); o < omega; o += stepsize) {
		k1 = dkdo(d, kn);
		std::cout << k1 << std::endl;
		k2 = dkdo(d, kn + stepsize * k1 / 2);
		k3 = dkdo(d, kn + stepsize * k2 / 2);
		k4 = dkdo(d, kn + stepsize * k3);
		kn += stepsize / 6 * (k1 + 2 * k2 + 2 * k3 + k4);	
		std::cout << "omega = " << o << " => k = " << kn << std::endl;
	}
	return kn;
}

void Ocean::generateMesh(std::vector<Vertex>& vertices, std::vector<Index>& indices) {
	vertices = std::vector<Vertex>();
	indices = std::vector<Index>();
	
	uint32_t n = pow(2, presubdivision) + 1, i;
	for (uint32_t x = 0; x < n; x++) {
		for (uint32_t y = 0; y < n; y++) {
			vertices.push_back({
				2 * scale * glm::vec3(float(x)/float(n - 1) - 0.5, 0, float(y)/float(n - 1) - 0.5),
				glm::vec2(float(x)/float(n - 1), float(y)/float(n - 1)),
				glm::vec3(0, 1, 0)
			});
			/*
			std::cout << "(" << vertices.back().position.x << ", " << vertices.back().position.y << ", " << vertices.back().position.z << ")" << std::endl;
			std::cout << "(" << vertices.back().uv.x << ", " << vertices.back().uv.y << ")" << std::endl;
			*/
		}
	}

	for (uint32_t x = 0; x < n - 1; x++) {
		for (uint32_t y = 0; y < n - 1; y++) {
			i = x * n + y;
			indices.push_back(i);
			indices.push_back(i + 1);
			indices.push_back(i + n);

			indices.push_back(i + 1);
			indices.push_back(i + n + 1);
			indices.push_back(i + n);
		}
	}

	if (sidewalls) {
		uint32_t vertend = vertices.size();
		for (uint32_t y = 0; y < n ; y++) {
			/*
			vertices.push_back({
				glm::vec3(vertices[y].position.x, 0, vertices[y].position.z),
				glm::vec2(0, 0),
				glm::vec3(1, 0, 0)
			});
			*/
			vertices.push_back({
				glm::vec3(vertices[y].position.x, -scale, vertices[y].position.z),
				glm::vec2(0, 0),
				glm::vec3(1, 1, 1)
			});
		}
		for (uint32_t y = 0; y < n - 1; y++) {
			indices.push_back(y);
			indices.push_back(vertend + y);
			indices.push_back(y + 1);

			indices.push_back(y + 1);
			indices.push_back(vertend + y);
			indices.push_back(vertend + y + 1);

			/*
			indices.push_back(vertend + 2 * y);
			indices.push_back(vertend + 2 * y + 1);
			indices.push_back(vertend + 2 * y + 2);

			indices.push_back(vertend + 2 * y + 2);
			indices.push_back(vertend + 2 * y + 1);
			indices.push_back(vertend + 2 * y + 1 + 2);
			*/
		}

		uint32_t newvertend = vertices.size();
		vertend -= 1;
		for (uint32_t y = 0; y < n ; y++) {
			vertices.push_back({
				glm::vec3(vertices[vertend - y].position.x, -scale, vertices[vertend - y].position.z),
				glm::vec2(0, 0),
				glm::vec3(-1, 0, 0)
			});
		}
		for (uint32_t y = 0; y < n - 1; y++) {
			indices.push_back(vertend - y);
			indices.push_back(newvertend + y);
			indices.push_back(vertend - y - 1);

			indices.push_back(vertend - y - 1);
			indices.push_back(newvertend + y);
			indices.push_back(newvertend + y + 1);
		}

		newvertend = vertices.size();
		for (uint32_t x = 0; x < n * n ; x += n) {
			vertices.push_back({
				glm::vec3(vertices[x].position.x, -scale, vertices[x].position.z),
				glm::vec2(0, 0),
				glm::vec3(0, 0, 1)
			});
		}
		for (uint32_t x = 0; x < n - 1; x++) {
			indices.push_back(x * n);
			indices.push_back((x + 1) * n);
			indices.push_back(newvertend + x);

			indices.push_back((x + 1) * n);
			indices.push_back(newvertend + x + 1);
			indices.push_back(newvertend + x);
		}

		newvertend = vertices.size();
		for (uint32_t x = 0; x < n * n ; x += n) {
			vertices.push_back({ glm::vec3(vertices[vertend - x].position.x, -scale, vertices[vertend - x].position.z), glm::vec2(0, 0),
				glm::vec3(0, 0, -1)
			});
		}
		for (uint32_t x = 0; x < n - 1; x++) {
			indices.push_back(vertend - x * n);
			indices.push_back(vertend - (x + 1) * n);
			indices.push_back(newvertend + x);

			indices.push_back(vertend - (x + 1) * n);
			indices.push_back(newvertend + x + 1);
			indices.push_back(newvertend + x);
		}
	}
}

void Ocean::renderDepthMap() {
	const VkCommandBuffer cb = gh->getInterimCommandBuffer();
	vkBeginCommandBuffer(cb, &interimcommandbufferbi);
	VkClearValue clear {1.};
	VkRenderPassBeginInfo rpbi {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		depthrenderpass,
		depthframebuffer,
		{{0, 0}, depthmap.extent},
		1,
		&clear
	};
	vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, depthpipeline.pipeline);
	glm::mat4 vp = glm::ortho<float>(
			floor->AABB[0].x, floor->AABB[1].x, 
			floor->AABB[1].z, floor->AABB[0].z, 
			0., floor->AABB[1].y -  floor->AABB[0].y);
	vp *= glm::lookAt<float>(glm::vec3(0., floor->AABB[1].y, 0.), glm::vec3(0.), glm::vec3(0., 0., -1.));
	vkCmdPushConstants(
		cb,
		depthpipeline.layout, 
		VK_SHADER_STAGE_VERTEX_BIT,
		0u,
		sizeof(DepthPCData),
		reinterpret_cast<void *>(&vp));
	VkDeviceSize dummyoffset = 0u;
	vkCmdBindVertexBuffers(cb, 0, 1, &floor->getVertexBufferPtr()->buffer, &dummyoffset);
	vkCmdBindIndexBuffer(cb, floor->getIndexBufferPtr()->buffer, 0u, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(cb, floor->getIndexBufferPtr()->size / sizeof(Index), 1u, 0u, 0u, 0u);
	vkCmdEndRenderPass(cb);
	vkEndCommandBuffer(cb);
	gh->submitInterimCB();
}

void Ocean::generateDepthMap() {
	// these shoaling displacement map extrema are store in depth map texels, to be converted later
	glm::ivec2 shoalingmin = glm::ivec2(depthmap.extent.width, depthmap.extent.height), 
		shoalingmax = glm::ivec2(0);

	float *data = new float[depthmap.extent.width * depthmap.extent.height];
	for (uint32_t x = 0; x < depthmap.extent.width; x++) {
		for (uint32_t y = 0; y < depthmap.extent.height; y++) {
			data[x * depthmap.extent.height + y] = sqrt(pow(float(x) / float(depthmap.extent.width), 2.) + pow(float(y) / float(depthmap.extent.height), 2.)) 
				* sqrt(float(x) / float(depthmap.extent.width) * 10.) 
				* 10. - 30.; 

			for (const Wave& w : waves) {
				if (w.wavetype == WAVE_TYPE_LINEAR) {
					if ((data[x * depthmap.extent.height + y] <= 0)
						&& ((w.linear.H / -data[x * depthmap.extent.height + y]) > 0.78)) {
						if (x < shoalingmin.x) shoalingmin.x = x;
						if (y < shoalingmin.y) shoalingmin.y = y;
						if (x > shoalingmax.x) shoalingmax.x = x;
						if (y > shoalingmax.y) shoalingmax.y = y;
					}
				}
			}
		}
	}
	gh->updateImage(depthmap, reinterpret_cast<void*>(data));
	delete[] data;

	computepcdata.dmuvmin = glm::vec2((float)shoalingmin.x / (float)depthmap.extent.width,
			(float)shoalingmin.y / (float)(depthmap.extent.height - 1));
	computepcdata.dmuvmax = glm::vec2((float)shoalingmax.x / (float)depthmap.extent.height,
			(float)shoalingmax.y / (float)(depthmap.extent.height - 1));
	computepcdata.dmuvmin = glm::vec2(computepcdata.dmuvmin.y, computepcdata.dmuvmin.x);
	computepcdata.dmuvmax = glm::vec2(computepcdata.dmuvmax.y, computepcdata.dmuvmax.x);
	graphicspcdata.dmuvmin = computepcdata.dmuvmin;
	graphicspcdata.dmuvmax = computepcdata.dmuvmax;

	shoalingcomputepcdata.worldscale = scale * (computepcdata.dmuvmax - computepcdata.dmuvmin);
	shoalingcomputepcdata.worldoffset = scale * (computepcdata.dmuvmin - glm::vec2(0.5));
}
