#include "Mesh.h"

#define HEIGHT_MAP_RESOLUTION 2048
#define DISPLACEMENT_MAP_RESOLUTION 512
#define DEPTH_MAP_RESOLUTION 16 // in texels/m
#define MAX_SHOALING_WAVES 8;

typedef struct OceanGraphicsPCData {
	glm::mat4 cameravp;
	glm::vec2 dmuvmin, dmuvmax;
	alignas(16) glm::vec3 camerapos;
} OceanGraphicsPCData;

typedef struct OceanComputePCData {
	float t;
	alignas(16) glm::vec2 dmuvmin, dmuvmax;
} OceanComputePCData;

typedef struct OceanShoalingComputePCData {
	glm::vec2 worldscale, worldoffset;
	float t;
} OceanShoalingComputePCData;

typedef struct DepthPCData {
	glm::mat4 cameravp;
} DepthPCData;

// in GLSL, we can use aliasing to access different kinds of waves accurately!
typedef enum WaveType {
	WAVE_TYPE_LINEAR
} WaveType;

// function type hard to implement given that GLSL doesn't have function pointers
typedef enum DepthType {
	DEPTH_TYPE_CONSTANT,
	// DEPTH_TYPE_FUNCTION,
	DEPTH_TYPE_MAPPED
} DepthType;

// typedef std::function<float (float, float)> depthFunc;

typedef struct DepthData {
	DepthType type;
	union {
		float d;
		//depthFunc df;
		ImageInfo dm;
	};
} DepthData;

// as we implement depth mapping, we may need to rethink how we calculate and
// access members that fluctuate with depth
typedef struct LinearWaveData {
	glm::vec2 k = glm::vec2(0.);
	float H = 0., omega = 0., d = 0.;
	
	ImageInfo kmap = {}; // this shouldn't get sent over to the shader

	LinearWaveData(float height, float length, float depth, glm::vec2 khat) {
		H = height;
		float Kmag = 6.28 / length;
		k = Kmag * glm::normalize(khat);
		d = depth;
		omega = sqrt(9.8 * Kmag * tanh(Kmag * d));
	}

	void addkMap(GH* g, ImageInfo& depth) {
		kmap.extent = {depth.extent.width, depth.extent.height};
		kmap.format = VK_FORMAT_R16G16_SFLOAT;
		kmap.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		kmap.layout = VK_IMAGE_LAYOUT_GENERAL;
		g->createImage(kmap);
	}

	void print() {
		std::cout << "LinearWaveData {\n\tH = "
			<< H << "\n\tomega = " 
			<< omega << "\n\td = "
			<< d << "\n\tk = {" << k.x << ", " << k.y << "}\n}" << std::endl;
	}
} LinearWaveData;

typedef struct Wave {
	WaveType wavetype;
	DepthType depthtype;
	union {
		LinearWaveData linear;
	};

	Wave(LinearWaveData l, DepthType d) {
		linear = l;
		wavetype = WAVE_TYPE_LINEAR;
		depthtype = d;
	}
} Wave;

/* 
 * if/when we implement any other kind of object that is rendered, we should probably have a subclass that ensures
 * things like having a pipeline & command buffer recording functions.
 */
class Ocean : public Drawable {
public:
	static PipelineInfo graphicspipeline, computepipeline, propertycomputepipeline, shoalingcomputepipeline;
	PipelineInfo depthpipeline;
	VkRenderPass depthrenderpass;
	VkFramebuffer depthframebuffer;
	Mesh* floor;

	Ocean();
	Ocean(GH* g);
	~Ocean();

	OceanGraphicsPCData * getGraphicsPCDataPtr() {return &graphicspcdata;}
	OceanComputePCData * getComputePCDataPtr() {return &computepcdata;}
	OceanShoalingComputePCData * getShoalingComputePCDataPtr() {return &shoalingcomputepcdata;}
	VkDescriptorSet getComputeDescriptorSet() {return computedescriptorset;}
	VkDescriptorSet getPropertyComputeDescriptorSet() {return propertycomputedescriptorset;}
	VkDescriptorSet getShoalingComputeDescriptorSet() {return shoalingcomputedescriptorset;}

	// Also inits heightmapsampler
	static void initGraphicsPipeline();
	static void initComputePipeline();
	void initDepthPipeline();
	static void terminatePipelines();
	static void recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordPropertyComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordShoalingComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	void attachEnvMap(ImageInfo& ii, VkSampler s);

	static std::vector<Wave> piersonMoskowitzSample(uint8_t n);

private:
	BufferInfo wavebuffer;
	ImageInfo heightmap, depthmap, displacementmap;
	OceanGraphicsPCData graphicspcdata;
	OceanComputePCData computepcdata;
	OceanShoalingComputePCData shoalingcomputepcdata;
	VkDescriptorSet computedescriptorset, propertycomputedescriptorset, shoalingcomputedescriptorset;
	float scale;
	uint8_t presubdivision;
	bool sidewalls;

	std::vector<Wave> waves;

	void initRenderpass();
	void terminateRenderpass();
	void initFramebuffer();
	void terminateFramebuffer();
	void initBuffers();
	void terminateBuffers();
	void initDescriptorSets();
	void terminateDescriptorSets();

	static float piersonMoskowitz(float f, float U10);

	static float inverseDispersionRelation(float omega, float d);

	void generateMesh(std::vector<Vertex>& vertices, std::vector<Index>& indices);

	void renderDepthMap();

	// below is in lieu of above, ideally we're able to import in the future
	void generateDepthMap();
};
