#include "GraphicsHandler.h"

// TODO: refine inheritance

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
