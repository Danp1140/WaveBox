#include "GraphicsHandler.h"

typedef enum DTHGraphicsFlagBits {
	DTH_GRAPHICS_FLAG_SSRR = 0x00000001,
	DTH_GRPAHICS_FLAG_CUBEMAP_REFLECTION = 0x00000002,
	DTH_GRAPHICS_FLAG_NO_DIFFUSE_TEXTURE = 0x00000004
} DTHGraphicsFlagBits;

typedef uint32_t DTHGraphicsFlags;

typedef struct DTHGraphicsPCData {
	glm::mat4 cameravp;
	alignas(16) glm::vec3 camerapos;
	DTHGraphicsFlags flags;
} DTHGraphicsPCData;

class Drawable {
public:
	~Drawable();

	// TODO: consider making const *
	BufferInfo * getVertexBufferPtr() {return &vertexbuffer;}
	BufferInfo * getIndexBufferPtr() {return &indexbuffer;}
	VkDescriptorSet getGraphicsDescriptorSet() {return graphicsdescriptorset;}

	static void initPipelines();
	static void terminatePipelines();
	static void terminateSamplers();

protected:
	static GH* gh;
	// DTH for dynamic tessellation with height map
	static PipelineInfo DTHgraphicspipeline;
	static VkSampler heightmapsampler;
	BufferInfo vertexbuffer, indexbuffer;
	VkDescriptorSet graphicsdescriptorset;

	Drawable() {}
	Drawable(GH* g);

	static void initSamplers();
	virtual void initBuffers() {}
	// why can't we have a default here that does the default destruction using gh's method?
	virtual void terminateBuffers() {}
	virtual void initDescriptorSets() {}
	virtual void terminateDescriptorSets() {}
};
