#include "Mesh.h"

#define HEIGHT_MAP_RESOLUTION 1024
#define DEPTH_MAP_RESOLUTION 16 // in texels/m
#define MAX_SHOALING_WAVES 8;

#define JACOBI_NOME_APPROXIMATION_ORDER 10
const float jacobitaylorexpcoeffs[JACOBI_NOME_APPROXIMATION_ORDER] = {
	1.591003453790792180,
	0.416000743991786912,
	0.245791514264103415,
	0.179481482914906162,
	0.144556057087555150,
	0.123200993312427711,
	0.108938811574293531,
	0.098853409871592910,
	0.091439629201749751,
	0.085842591595413900
};

typedef enum OceanComputeFlagBits {
	OCEAN_COMPUTE_FLAG_DEPTH_MAP_CHANGE = 0x00000001
} OceanComputeFlagBits;

typedef uint32_t OceanComputeFlags;

typedef struct OceanComputePCData {
	float t;
	OceanComputeFlags flags;
} OceanComputePCData;

typedef struct DepthPCData {
	glm::mat4 cameravp;
} DepthPCData;

// in GLSL, we can use aliasing to access different kinds of waves accurately!
typedef enum WaveType {
	WAVE_TYPE_LINEAR,
	WAVE_TYPE_CNOIDAL
} WaveType;

// function type hard to implement given that GLSL doesn't have function pointers
typedef enum DepthType {
	DEPTH_TYPE_CONSTANT,
	// DEPTH_TYPE_FUNCTION,
	DEPTH_TYPE_MAPPED // still unsure if/how we use these....
} DepthType;

typedef struct DepthData {
	DepthType type;
	union {
		float d;
		ImageInfo dm;
	};
} DepthData;

// as we implement depth mapping, we may need to rethink how we calculate and
// access members that fluctuate with depth
typedef struct LinearWaveData {
	glm::vec2 k = glm::vec2(0.);
	float H = 0., omega = 0., d = 0.;
	float padding;
	
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

#define CNOIDAL_WAVE_NUM_MACLAURIN_TERMS 7
typedef struct CnoidalSupplementalData {
	uint32_t correspondingwaveidx = -1u;
	float m = 0, bigK = 1.57;
	float snMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS], 
	      cnMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS], 
	      dnMaclaurinTerms[CNOIDAL_WAVE_NUM_MACLAURIN_TERMS];

	CnoidalSupplementalData(uint32_t i, float ellipticalm) {
		correspondingwaveidx = i;
		setM(ellipticalm);
	}

	void setM(float ellipticalm) {
		m = ellipticalm;
		snMaclaurinTerms[0] = 1;
		snMaclaurinTerms[1] = - (1 + m) / 6;
		snMaclaurinTerms[2] = (1 + pow(m, 2) + 14 * m) / 120;
		snMaclaurinTerms[3] = - (1 + pow(m, 3) + 135 * m * (1 + m)) / 5040;
		snMaclaurinTerms[4] = (1 + pow(m, 5) + 11069 * m * (1 + pow(m, 3)) 
			 + 165826 * pow(m, 2) * (1 + m)) / 39916800;
		snMaclaurinTerms[5] = (1 + pow(m, 6) + 99642 * m * (1 + pow(m, 4)) 
			 + 4494351 * pow(m, 2) * (1 + pow(m, 2)) + 13180268 * pow(m, 3)) / 6227020800;
		snMaclaurinTerms[6] = (1 + pow(m, 7) + 896803 * m * (1 + pow(m, 5))
			 + 116294673 * pow(m, 2) * (1 + pow(m, 3)) + 834687179 * pow(m, 3) * (1 + m)) / 1307674368000;

		cnMaclaurinTerms[0] = 1;
		cnMaclaurinTerms[1] = - 0.5;
		cnMaclaurinTerms[2] = (1 + 4 * m) / 24;
		cnMaclaurinTerms[3] = - (1 + 44 * m + 16 * pow(m, 2)) / 720;
		cnMaclaurinTerms[4] = (1 + 408 * m + 912 * pow(m, 2) + 64 * pow(m, 3)) / 40320;
		cnMaclaurinTerms[5] = (1 + 33212 * m + 870640 * pow(m, 2) + 1538560 * pow(m, 3) 
			 + 249328 * pow(m, 4) + 1024 * pow(m, 5)) / 479001600;
		cnMaclaurinTerms[6] = (1 + 298932 * m + 22945056 * pow(m, 2) + 106923008 * pow(m, 3)
			 + 65008896 * pow(m, 4) + 4180992 * pow(m, 5) + 4096 * pow(m, 6)) / 87178291200;

		dnMaclaurinTerms[0] = 1;
		dnMaclaurinTerms[1] = - m / 2;
		dnMaclaurinTerms[2] = (4 * m + pow(m, 2)) / 24;
		dnMaclaurinTerms[3] = - (16 * m + 44 * pow(m, 2) + pow(m, 3)) / 720;
		dnMaclaurinTerms[4] = (64 * m + 912 * pow(m, 2) + 408 * pow(m, 3) + pow(m, 4)) / 40320;
		dnMaclaurinTerms[5] = (1024 * m + 259328 * pow(m, 2) + 1538560 * pow(m, 3) 
			 + 870640 * pow(m, 4) + 33212 * pow(m, 5) + pow(m, 6)) / 479001600;
		dnMaclaurinTerms[6] = (pow(m, 7) + 298932 * pow(m, 6) + 22945056 * pow(m, 5) + 106923008 * pow(m, 4)
			 + 65008896 * pow(m, 3) + 4180992 * pow(m, 2) + 4096 * m) / 87178291200;
		
		if (m == 0) bigK = 1.57;
		else if (m == 1) bigK = std::numeric_limits<float>::infinity();
		else if (m < 0.9) {
			bigK = 1 + pow(m, 2) / 4 + 9 * pow(m, 4) / 64 + 25 * pow(m, 6) / 256 + 1225 * pow(m, 8) / 16384
				 + 15875 * pow(m, 10) / 262144 + 53325 * pow(m, 12) / 1048576 + 2936375 * pow(m, 14) / 67108864;
			bigK *= 1.57;
		}
		else {
			const float mprime = 1 - m;
			const float m0 = 0.05;
			float qprime = mprime / 16 + pow(mprime, 2) / 32 + 21 * pow(mprime, 3) / 1024
				 + 31 * pow(mprime, 4) / 2048 + 6257 * pow(mprime, 5) / 524288
				 + 10293 * pow(mprime, 6) / 1048576 + 279025 * pow(mprime, 7) / 33554432;
			float bigKprime = 0;
			for (uint8_t i = 0; i < JACOBI_NOME_APPROXIMATION_ORDER; i++) {
				bigKprime += jacobitaylorexpcoeffs[i] * pow(mprime - m0, i);
			}
			bigK = -log(qprime) * bigKprime / 3.14;
		}
	}
} CnoidalSupplementalData;

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

typedef struct BaseWaveData {
	WaveType wavetype;
	DepthType depthtype;
	glm::vec2 k = glm::vec2(0.);
	float H = 0., omega = 0., d = 0.;
	float padding;
	
	ImageInfo kmap = {}; // this shouldn't get sent over to the shader

	BaseWaveData(WaveType wtype, DepthType dtype, float height, float length, float depth, glm::vec2 khat) {
		wavetype = wtype;
		depthtype = dtype;
		H = height;
		float Kmag = 6.28 / length;
		k = Kmag * glm::normalize(khat);
		d = depth;
		omega = sqrt(9.8 * Kmag * tanh(Kmag * d));
	}

	size_t sizeForShaders() {return offsetof(BaseWaveData, kmap);}

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
} BaseWaveData;


/* 
 * if/when we implement any other kind of object that is rendered, we should probably have a subclass that ensures
 * things like having a pipeline & command buffer recording functions.
 */
class Ocean : public Drawable {
public:
	static PipelineInfo graphicspipeline, computepipeline, propertycomputepipeline;
	PipelineInfo depthpipeline;
	VkRenderPass depthrenderpass;
	VkFramebuffer depthframebuffer;
	Mesh* floor;

	Ocean();
	Ocean(GH* g);
	~Ocean();

	DTHGraphicsPCData * getGraphicsPCDataPtr() {return &graphicspcdata;}
	OceanComputePCData * getComputePCDataPtr() {return &computepcdata;}
	VkDescriptorSet getComputeDescriptorSet() {return computedescriptorset;}
	VkDescriptorSet getPropertyComputeDescriptorSet() {return propertycomputedescriptorset;}

	// Also inits heightmapsampler
	static void initGraphicsPipeline();
	static void initComputePipeline();
	void initDepthPipeline();
	static void terminatePipelines();
	static void recordGraphicsCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	static void recordPropertyComputeCommandBuffer(VkCommandBuffer& cb, cbRecData data);
	void attachEnvMap(ImageInfo& ii, VkSampler s);

	static std::vector<Wave> piersonMoskowitzSample(uint8_t n);

private:
	BufferInfo wavebuffer, cnoidalsupplementalbuffer;
	ImageInfo heightmap, depthmap;
	DTHGraphicsPCData graphicspcdata;
	OceanComputePCData computepcdata;
	VkDescriptorSet computedescriptorset, propertycomputedescriptorset;
	float scale;
	uint8_t presubdivision;
	bool sidewalls;

	std::vector<BaseWaveData> waves;
	std::vector<CnoidalSupplementalData> cndata;

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
