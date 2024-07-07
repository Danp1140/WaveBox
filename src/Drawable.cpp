#include "Drawable.h"

GH* Drawable::gh = nullptr;
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

