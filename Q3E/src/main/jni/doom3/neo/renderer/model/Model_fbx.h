#ifndef _MODEL_FBX_H
#define _MODEL_FBX_H

class idMd5MeshFile;
class idMd5AnimFile;

#define FBX_MAGIC {'K', 'a', 'y', 'd', 'a', 'r', 'a', ' ', 'F', 'B', 'X', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 0x20, 0x20, 0x00, 0x1a, 0x00} // 23
#define FBX_SEC 46186158000.0

#pragma pack( push, 1 )
typedef struct fbxHeader_s
{
    char magic[23];
    uint32_t fbx_version;
} fbxHeader_t;

typedef struct fbxElem32_s // <IIIB
{
    uint32_t end_offset;
    uint32_t prop_count; // element properties count
    uint32_t _prop_length;
    byte elem_id_size; // element ID byte size
    idStr elem_id; // element ID
} fbxElem32_t;

typedef struct fbxElem64_s // <QQQB
{
    uint64_t end_offset;
    uint64_t prop_count; // element properties count
    uint64_t _prop_length;
    byte elem_id_size; // element ID byte size
    idStr elem_id; // element ID
} fbxElem64_t;

typedef struct fbxArrayParam_s // <III
{
    uint32_t length; // num elements
    uint32_t encoding; // is compressed
    uint32_t comp_len; // byte size
} fbxArrayParam_t;

typedef enum fbxArrayType_e
{
    FBX_ARRAY_BOOL = 'b', // bool
    FBX_ARRAY_BYTE = 'c', // ubyte
    FBX_ARRAY_INT32 = 'i', // int
    FBX_ARRAY_INT64 = 'l', // long
    FBX_ARRAY_FLOAT32 = 'f', // float
    FBX_ARRAY_FLOAT64 = 'd', // double
} fbxArrayType_t;

typedef enum fbxDataType_e
{
    FBX_DATA_BYTE = 'Z', // byte <b
    FBX_DATA_INT16 = 'Y', // 16 bit int <h
    FBX_DATA_BOOL = 'B', // 1 bit bool (yes/no) ?
    FBX_DATA_CHAR = 'C', // char <c
    FBX_DATA_INT32 = 'I', // 32 bit int <i
    FBX_DATA_FLOAT32 = 'F', // 32 bit float <f
    FBX_DATA_FLOAT64 = 'D', // 64 bit float <d
    FBX_DATA_INT64 = 'L', // 64 bit int <q
    FBX_DATA_BINARY = 'R', // binary data
    FBX_DATA_STRING = 'S', // string data
} fbxDataType_t;

typedef struct fbxArray_s {
    void *ptr;
    unsigned int length;
    unsigned int stride;
    unsigned int count;
} fbxArray_t;

typedef union fbxData_u
{
    bool z;
    byte b;
    short h;
    int i;
    float f;
    double d;
    int64_t l;
    char c;
    unsigned u;
    fbxArray_t a;
} fbxData_t;

typedef struct fbxProperty_s
{
    byte type;
    fbxData_t data;
} fbxProperty_t;

typedef struct fbxNode_s
{
    idStr elem_id;
    idList<fbxProperty_t> elem_props;
    idList<fbxNode_s> *elem_subtree;
} fbxNode_t;

#pragma pack( pop )

class idModelFbx
{
    public:
        idModelFbx(void);
        ~idModelFbx(void);
        bool Parse(const char *fbxPath);
        bool ParseAscii(const char *fbxPath);
        bool ParseBinary(const char *fbxPath);
        void Print(void) const;
        const char * GetAnim(unsigned int index) const;
        int GetAnimCount(void) const;
        int GetAnimIndex(const char *name) const;
        bool ToMd5Mesh(idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
        bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
        bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
        int ToMd5AnimList(idList<idMd5AnimFile> &md5anim, idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
#ifdef _MODEL_OBJ
        bool ToObj(objModel_t &objModel) const;
#endif

    private:
        void Clear(void);
        void MarkType(int type);
        bool IsTypeMarked(int type) const;
        int GroupTriangle(idList<idList<idDrawVert> > &verts, idList<idList<int> > &faces, idStrList &mats, bool keepDup) const;

private: // binary fbx loader
        int ReadHeader(fbxHeader_t &header);
        int read_elem_start64(fbxElem64_t &elem);
        int read_elem_start32(fbxElem32_t &elem);
        uint64_t read_fbx_elem_start(uint64_t &end_offset, uint64_t &prop_count, idStr &elem_id);
        int read_array_params(fbxArrayParam_t &param);
        int _create_array(fbxArray_t &data, int length, int array_type, int array_stride, bool array_byteswap);
        int _decompress_and_insert_array(fbxArray_t &ret, const fbxArray_t &compressed_data, int length, int array_type, int array_stride, bool array_byteswap);
        int unpack_array(fbxArray_t &data, int array_type, int array_stride, bool array_byteswap);
        int read_elem(fbxNode_t &object, int tell_file_offset = 0);
        int read_array_dict(fbxArray_t &data, int array_type);
        int read_data_dict(fbxData_t &data, int data_type);
        void Print(const idList<fbxNode_t> &root) const;

private: // ascii fbx loader
		void AsciiSkipComment(void);
		int AsciiParseProperty(idList<fbxProperty_t> &props);
		int AsciiParseNumber(int64_t &l, double &d);
		int AsciiParseArray(fbxArray_t &array);
		int AsciiParseChildren(idList<fbxNode_t> &children);
		bool AsciiParseNode(fbxNode_t &node);
		int ReadToken(idToken *token, bool onLine = false); 

	private:
#define fbxLayerElement_TEMPLATE(N, T) struct fbxLayerElement##N { \
        int id; \
        idStr MappingInformationType; \
        idStr ReferenceInformationType; \
        idList<T> N##s; \
        idList<int> N##Index; \
        bool ByVertex(void) const { return !idStr::Icmp("ByPolygonVertex", MappingInformationType); } \
        bool ByPolygon(void) const { return !idStr::Icmp("ByPolygon", MappingInformationType); } \
        bool IsIndexMap(void) const { return !idStr::Icmp("IndexToDirect", ReferenceInformationType); } \
        bool IsDirect(void) const { return !idStr::Icmp("Direct", ReferenceInformationType); } \
        bool UsingIndexMap(void) const { return N##Index.Num() > 0 || IsIndexMap(); } \
        bool UsingData(void) const { return N##Index.Num() == 0 || IsDirect(); } \
        const T * Get(int n, int index, int stride, int numVerts, int numIndexes, T ret[] = NULL) const { \
            const T *ptr = NULL; \
            if(UsingIndexMap()) { \
                int idx = stride * N##Index[n]; \
                ptr = &N##s[idx]; \
            } else { \
                if(numIndexes * stride == N##s.Num()) { \
                    ptr = &N##s[stride * n]; \
                } else { \
                    int idx = index; \
                    if(idx < 0) idx = -(idx+1); \
                    idx *= stride; \
                    if((stride == 3 && N##s.Num() == numVerts) || (N##s.Num() / stride == numVerts / 3)) { \
                        ptr = &N##s[idx]; \
                    } else { \
                        if(idx + (stride - 1) < N##s.Num()) { \
                            ptr = &N##s[idx]; \
                        } \
                    } \
                } \
            } \
            if(ptr && ret) { \
                for(int i = 0; i < stride; i++) \
                    ret[i] = ptr[i]; \
            } \
            return ptr; \
        } \
    }

#define fbxLayerElement_TEMPLATE_TYPE(T) fbxLayerElement##T

		fbxLayerElement_TEMPLATE(Normal, float);
		fbxLayerElement_TEMPLATE(Color, float);
		fbxLayerElement_TEMPLATE(UV, float);
		fbxLayerElement_TEMPLATE(Material, int);

		struct fbxLayerElement {
			idStr Type;
			unsigned int TypedIndex;
		};

		struct fbxLayer {
			int id;
			idList<fbxLayerElement> elements;
		};

		struct fbxConnection {
			idStr type;
			int64_t from, to;
			idStr property;

			fbxConnection(void) : from(0), to(0) {}
			bool Convert(const fbxNode_t &conns);
		};

		enum { GEOM = 0, MODEL, MATERIAL, LIMB, CLUSTER, SKIN, CURVE, XFORM, ANIMLAYER, ANIMSTACK, MODELNULL, BINDPOSE, };
		struct fbxBaseNode;
        struct fbxGeometry;
        struct fbxLimbNode;
        struct fbxAnimationCurveNode;
        struct fbxCluster;
        struct fbxAnimationStack;
        struct fbxAnimationCurve;
        struct fbxMesh;

		struct fbxConnectionNode {
			const fbxBaseNode *node;
			const char *property;
		};

		struct fbxBaseNode {
			int64_t id;
			idStr name;
			idList<fbxConnectionNode> connections;
			idList<fbxConnectionNode> parents;

			fbxBaseNode(void) : id(0) {}

			virtual int Type(void) const = 0;
			virtual bool Convert(const fbxNode_t &node);
			void Connect(fbxBaseNode *node, const char *property);
			template <class T>
			const T * FindConnection(int type) const;
			template <class T>
			const T * FindConnection(int type, const char *name) const;
			template <class T>
			int FindConnections(idList<const T *> &ret, int type) const;
			template <class T>
			const T * FindParent(int type) const;
			template <class T>
			const T * FindParent(int type, const char *name) const;
            virtual void Clear(void);
            static const char * TypeName(int type);
		};

        struct fbxBaseTransformNode : fbxBaseNode {
            float Lcl_Translation[3];
            float Lcl_Rotation[3];
            float Lcl_Scaling[3];

            fbxBaseTransformNode(void) : fbxBaseNode() {
                Lcl_Translation[0] = Lcl_Translation[1] = Lcl_Translation[2] = 0.0f;
                Lcl_Rotation[0] = Lcl_Rotation[1] = Lcl_Rotation[2] = 0.0f;
                Lcl_Scaling[0] = Lcl_Scaling[1] = Lcl_Scaling[2] = 1.0f;
            }
            virtual bool Convert(const fbxNode_t &node);
            virtual void Clear(void);
        };

        /*
         * ROOT -> |Model::Null| -> Model::LimbNode
         *         |           | -> AnimationCurveNode::AnimCurveNode::<T|R|S>...
         */
        struct fbxNull : fbxBaseTransformNode {
            virtual int Type(void) const { return MODELNULL; }
            const fbxLimbNode * RootLimb(void) const { return FindConnection<fbxLimbNode>(LIMB); }
            int AnimationCurveNode(idList<const fbxAnimationCurveNode *> &list) const { return FindConnections<fbxAnimationCurveNode>(list, XFORM); }
        };

        /*
         * Model::Mesh -> |Material|
         */
        struct fbxMaterial : fbxBaseNode {
            virtual int Type(void) const { return MATERIAL; }
            const fbxMesh * Mesh(void) const { return FindParent<fbxMesh>(MODEL); }
        };

        /*
         * ROOT -> |Model::Mesh| -> Geometry::Mesh
         *         |           | -> Material...
         */
        struct fbxMesh : fbxBaseTransformNode {
            virtual int Type(void) const { return MODEL; }
            const fbxGeometry * Geometry(void) const { return FindConnection<fbxGeometry>(GEOM); }
            int Material(idList<const fbxMaterial *> &list) const { return FindConnections<fbxMaterial>(list, MATERIAL); }
        };

        /*
         * Deformer::Cluster -> |Model::LimbNode| -> Model::LimbNode...
         * Model::LimbNode   -> |               | -> AnimationCurveNode::AnimCurveNode::<T|R|S>...
         */
		struct fbxLimbNode : fbxBaseTransformNode {
            virtual int Type(void) const { return LIMB; }
            const fbxLimbNode * Parent(void) const { return FindParent<fbxLimbNode>(LIMB); }
            const fbxCluster * Cluster(void) const { return FindParent<fbxCluster>(CLUSTER); }
            int Children(idList<const fbxLimbNode *> &list) const { return FindConnections<fbxLimbNode>(list, LIMB); }
            int AnimationCurveNode(idList<const fbxAnimationCurveNode *> &list) const { return FindConnections<fbxAnimationCurveNode>(list, XFORM); }
        };

        /*
         * Geometry::Mesh -> |Deformer::Skin| -> Deformer::Cluster...
         */
        struct fbxSkin : fbxBaseNode {
            virtual int Type(void) const { return SKIN; }
            const fbxGeometry * Geometry(void) const { return FindParent<fbxGeometry>(GEOM); }
            int Cluster(idList<const fbxCluster *> &list) const { return FindConnections<fbxCluster>(list, CLUSTER); }
        };

        /*
         * Geometry::Skin -> |Deformer::Cluster| -> Model::LimbNode
         */
		struct fbxCluster : fbxBaseNode {
            idList<int> Indexes;
            idList<float> Weights;
            idList<float> Transform; // 16
            idList<float> TransformLink; // 16
            //idList<float> TransformAssociateModel; // 16

            virtual int Type(void) const { return CLUSTER; }
            virtual bool Convert(const fbxNode_t &node);
            const fbxLimbNode * LimbNode(void) const { return FindConnection<fbxLimbNode>(LIMB); }
            const fbxSkin * Skin(void) const { return FindParent<fbxSkin>(SKIN); }
        };

        /*
         * Model::Mesh -> |Geometry::Mesh| -> Deformer::Skin
         */
		struct fbxGeometry : fbxBaseNode {
            idList<float> Vertices;
            idList<int> PolygonVertexIndex;
            idList<int> Edges;
            idList<fbxLayerElement_TEMPLATE_TYPE(Normal)> LayerElementNormal;
            idList<fbxLayerElement_TEMPLATE_TYPE(Color)> LayerElementColor;
            idList<fbxLayerElement_TEMPLATE_TYPE(UV)> LayerElementUV;
            idList<fbxLayerElement_TEMPLATE_TYPE(Material)> LayerElementMaterial;
            fbxLayer Layer; // only first one

            fbxGeometry(void) : fbxBaseNode() {
                Layer.id = -1;
            }
            virtual int Type(void) const { return GEOM; }
            virtual bool Convert(const fbxNode_t &node);
            virtual void Clear(void);
            const fbxSkin * Skin(void) const { return FindConnection<fbxSkin>(SKIN); }
            const fbxMesh * Mesh(void) const { return FindParent<fbxMesh>(MODEL); }
        };

        struct fbxPoseNode {
            int64_t Node;
            idMat4 Matrix;

            fbxPoseNode(void) : Node(0) {
                Matrix.Identity();
            }
            bool Convert(const fbxNode_t &node);
        };

        /*
         * |Pose::BindPose|
         */
		struct fbxBindPose : fbxBaseNode {
            idList<fbxPoseNode> PoseNode;

            virtual int Type(void) const { return BINDPOSE; }
            bool Convert(const fbxNode_t &node);
            void Clear(void);
        };

        /*
         * AnimationStack::AnimStack -> |AnimationLayer::AnimLayer| -> AnimationCurveNode::AnimCurveNode::<T|R|S>...
         */
        struct fbxAnimationLayer : fbxBaseNode {
            virtual int Type(void) const { return ANIMLAYER; }
            const char * AnimName(void) const;
			const fbxAnimationStack * AnimationStack(void) const { return FindParent<fbxAnimationStack>(ANIMSTACK); }
			int AnimationCurveNode(idList<const fbxAnimationCurveNode *> &list) const { return FindConnections<fbxAnimationCurveNode>(list, XFORM); }
        };

        /*
         * |AnimationStack::AnimStack| -> AnimationLayer::AnimLayer...
         */
        struct fbxAnimationStack : fbxBaseNode {
            int64_t LocalStop;
            int64_t ReferenceStop;

            fbxAnimationStack(void) : fbxBaseNode(), LocalStop(-1), ReferenceStop(-1) {}
            virtual int Type(void) const { return ANIMSTACK; }
            virtual bool Convert(const fbxNode_t &node);
			const fbxAnimationLayer * AnimationLayer(void) const { return FindConnection<fbxAnimationLayer>(ANIMLAYER); }
            const char * AnimName(void) const;
			int NumFrames(int framerate = 24) const;
			float Seconds(void) const;
        };

        /*
         * Model::LimbNode           -> |AnimationCurveNode::AnimCurveNode::<T|R|S>| -> AnimationCurve::AnimCurve::(d|X,d|Y,d|Z)...
         * AnimationLayer::AnimLayer -> |                                          |
         */
        struct fbxAnimationCurveNode : fbxBaseNode {
            byte type; // T R S
            float x, y, z;

            fbxAnimationCurveNode(void) : fbxBaseNode(), type('\0'), x(0.0f), y(0.0f), z(0.0f) {}
            virtual int Type(void) const { return XFORM; }
            virtual bool Convert(const fbxNode_t &node);
			int AnimationCurves(idList<const fbxAnimationCurve *> &list) const { return FindConnections<fbxAnimationCurve>(list, CURVE); }
			const fbxLimbNode * LimbNode(void) const { return FindParent<fbxLimbNode>(LIMB); }
			const fbxAnimationLayer * AnimationLayer(void) const { return FindParent<fbxAnimationLayer>(ANIMLAYER); }
            const fbxAnimationCurve * AnimationCurve(int index) const;
        };

        /*
         * AnimationCurveNode::AnimCurveNode::<T|R|S>(d|X,d|Y,d|Z) -> |AnimationCurve::AnimCurve::|
         */
        struct fbxAnimationCurve : fbxBaseNode {
            //int KeyVer;
			float Default;
            idList<int64_t> KeyTime;
            idList<float> KeyValueFloat;
            //idList<int> KeyAttrFlags;
            //idList<float> KeyAttrDataFloat;
            //idList<int> KeyAttrRefCount;
			
            fbxAnimationCurve(void) : fbxBaseNode(), Default(0.0f) {}

            virtual int Type(void) const { return CURVE; }
            virtual bool Convert(const fbxNode_t &node);
			const fbxAnimationCurveNode * AnimationCurveNode(void) const { return FindParent<fbxAnimationCurveNode>(XFORM); }
			void GenFrameData(idList<float> &frames, int numFrames) const;
			float LinearData(int64_t time) const;
        };

		struct fbxObject {
            fbxNull Null;
            fbxMesh Mesh;

			fbxSkin Skin;
            fbxBindPose BindPose;
			fbxGeometry Geometry;
			idList<fbxLimbNode> LimbNode;
			idList<fbxCluster> Cluster;
			idList<fbxMaterial> Material;

            idList<fbxAnimationLayer> AnimationLayer;
            idList<fbxAnimationStack> AnimationStack;
            idList<fbxAnimationCurveNode> AnimationCurveNode;
            idList<fbxAnimationCurve> AnimationCurve;

			bool Convert(const fbxNode_t &objects);
            void Clear(void);
            int AnimCount(void) const;
            const char * AnimName(unsigned int index) const;
		};

		struct fbxModel {
			fbxObject Objects;
			idList<fbxConnection> Connections;
			idList<fbxBaseNode *> nodes;
            idList<fbxConnectionNode> root;

			bool Convert(const idList<fbxNode_t> &objects);
			bool ToConnections(const fbxNode_t &node);
			int AddNodes(void);
			void AddConnections(void);
            void Clear(void);
			fbxBaseNode * FindNode(int64_t id);
            const fbxBaseNode * FindNode(int64_t id) const;
            void Print(void) const;
		};

private: // binary fbx reader
        static void * AllocArrayData(fbxArray_t &data, unsigned int length);
        static void FreeArrayData(fbxArray_t &data);
        static void FreeProperty(fbxProperty_t &prop);
        static void FreeObject(fbxNode_t &object);
        static void PrintObject(int index, const fbxNode_t &object, int intent = 0);
		static const fbxNode_t * FindObject(const idList<fbxNode_t> &list, const char *name);
		static const fbxNode_t * FindChild(const fbxNode_t &object, const char *name);
		static bool PropertyIsTypes(const fbxNode_t &object, const char *types);
		static bool PropertyIsType(const fbxNode_t &object, int index, byte type);
		template <class T>
		static T GetProperty(const fbxNode_t &obj, int index, T def = (T)0);
        template <class T>
        static int CopyArrayToList(idList<T> &list, const fbxProperty_t &prop);
        template <class T>
        static int CopyArrayToArray(T array[], unsigned int length, const fbxProperty_t &prop);

    private:
        idFile *file;
		idLexer *lexer;
        unsigned int fbx_version;
        fbxModel model;
        int types;

        friend class idRenderModelStatic;
};



template <class T>
ID_INLINE const T * idModelFbx::fbxBaseNode::FindConnection(int type) const
{
    for(int i = 0; i < connections.Num(); i++)
    {
        if(connections[i].node->Type() == type)
            return (const T *)connections[i].node;
    }
    return NULL;
}

template <class T>
ID_INLINE const T * idModelFbx::fbxBaseNode::FindConnection(int type, const char *prop) const
{
    for(int i = 0; i < connections.Num(); i++)
    {
        const fbxConnectionNode &n = connections[i];
        if(n.node->Type() == type && !idStr::Icmp(prop, n.property))
            return (const T *)n.node;
    }
    return NULL;
}

template <class T>
ID_INLINE const T * idModelFbx::fbxBaseNode::FindParent(int type) const {
    for(int i = 0; i < parents.Num(); i++)
    {
        if(parents[i].node->Type() == type)
            return (const T *)parents[i].node;
    }
    return NULL;
}

template <class T>
ID_INLINE const T * idModelFbx::fbxBaseNode::FindParent(int type, const char *prop) const {
    for(int i = 0; i < parents.Num(); i++)
    {
        const fbxConnectionNode &n = parents[i];
        if(n.node->Type() == type && !idStr::Icmp(prop, n.property))
            return (const T *)n.node;
    }
    return NULL;
}

template <class T>
int idModelFbx::fbxBaseNode::FindConnections(idList<const T *> &ret, int type) const
{
	int num = 0;
	for(int i = 0; i < connections.Num(); i++) {
		const fbxConnectionNode &conn = connections[i];
		if(conn.node->Type() == type) {
			ret.Append((const T *)conn.node);
			num++;
		}
	}
	return num;
}



template <class T>
T idModelFbx::GetProperty(const fbxNode_t &obj, int index, T def) {
	if(index < 0)
		index = obj.elem_props.Num() + index;
	if(index < 0 || index >= obj.elem_props.Num())
		return def;
	const fbxProperty_t &prop = obj.elem_props[index];
	switch(prop.type)
	{
		case FBX_DATA_BOOL:
			return (T)prop.data.z;
		case FBX_DATA_BYTE:
			return (T)prop.data.b;
		case FBX_DATA_CHAR:
			return (T)prop.data.c;
		case FBX_DATA_INT16:
			return (T)prop.data.h;
		case FBX_DATA_INT32:
			return (T)prop.data.i;
		case FBX_DATA_INT64:
			return (T)prop.data.l;
		case FBX_DATA_FLOAT32:
			return (T)prop.data.f;
		case FBX_DATA_FLOAT64:
			return (T)prop.data.d;
		case FBX_ARRAY_BOOL:
		case FBX_ARRAY_BYTE:
		case FBX_ARRAY_INT32:
		case FBX_ARRAY_INT64:
		case FBX_ARRAY_FLOAT32:
		case FBX_ARRAY_FLOAT64:
		case FBX_DATA_STRING:
		case FBX_DATA_BINARY:
		default:
			common->Error("Access fbx property is not single value: %d\n", prop.type);
			return def;
	}
}

template <>
ID_INLINE const char * idModelFbx::GetProperty<>(const fbxNode_t &obj, int index, const char *def) {
	if(index < 0)
		index = obj.elem_props.Num() + index;
	if(index < 0 || index >= obj.elem_props.Num())
		return def;
	const fbxProperty_t &prop = obj.elem_props[index];
	switch(prop.type)
	{
		case FBX_DATA_STRING:
			return (const char *)prop.data.a.ptr;
		default:
			common->Error("Access fbx property is not string: %d\n", prop.type);
			return def;
	}
}

template <class T>
int idModelFbx::CopyArrayToList(idList<T> &list, const fbxProperty_t &prop) {
	list.SetNum(prop.data.a.count);
	switch(prop.type)
	{
		case FBX_ARRAY_BOOL:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const bool *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_BYTE:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const byte *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_INT32:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const int32_t *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_INT64:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const int64_t *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_FLOAT32:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const float *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_FLOAT64:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const double *)prop.data.a.ptr)[i];
			break;
		case FBX_DATA_STRING:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const char *)prop.data.a.ptr)[i];
			break;
		case FBX_DATA_BINARY:
			for(unsigned int i = 0; i < prop.data.a.count; i++)
				list[i] = (T)((const byte *)prop.data.a.ptr)[i];
			break;
		default:
			common->Error("Access fbx property is not array: %d\n", prop.type);
			return 0;
	}
	return list.Num();
}

template <class T>
int idModelFbx::CopyArrayToArray(T array[], unsigned int length, const fbxProperty_t &prop) {
	unsigned int maxlen = prop.data.a.count > length ? length : prop.data.a.count;
	switch(prop.type)
	{
		case FBX_ARRAY_BOOL:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const bool *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_BYTE:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const byte *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_INT32:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const int32_t *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_INT64:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const int64_t *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_FLOAT32:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const float *)prop.data.a.ptr)[i];
			break;
		case FBX_ARRAY_FLOAT64:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const double *)prop.data.a.ptr)[i];
			break;
		case FBX_DATA_STRING:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const char *)prop.data.a.ptr)[i];
			break;
		case FBX_DATA_BINARY:
			for(unsigned int i = 0; i < maxlen; i++)
				array[i] = (T)((const byte *)prop.data.a.ptr)[i];
			break;
		default:
			common->Error("Access fbx property is not array: %d\n", prop.type);
			return 0;
	}
	return maxlen;
}


#endif
