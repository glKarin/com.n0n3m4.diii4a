#include "Model_fbx.h"

#include <float.h>
#ifdef FLT_EPSILON
#undef FLT_EPSILON
#endif

#include "Model_md5mesh.h"
#include "Model_md5anim.h"
#include "../../framework/Unzip.h"

#define FBX_BLOCK_SENTINEL_LENGTH() (fbx_version < 7500 ? 13 : 25)

extern int LongSwap(int l); // in idLib/Lib.cpp
extern int64_t LongLongSwap(int64_t l);
extern idQuat fromangles(const idVec3 &rot);

static idQuat fromdegrees(const idVec3 &rot) { return fromangles(rot * (idMath::PI / 180.0f)); }

static void SplitMatrixTransform(const idMat4 &mat, idVec3 &origin, idQuat &quat)
{
    origin[0] = mat[3][0];
    origin[1] = mat[3][1];
    origin[2] = mat[3][2];

    idMat3 m3;
    m3[0][0] = mat[0][0];
    m3[1][0] = mat[0][1];
    m3[2][0] = mat[0][2];
    m3[0][1] = mat[1][0];
    m3[1][1] = mat[1][1];
    m3[2][1] = mat[1][2];
    m3[0][2] = mat[2][0];
    m3[1][2] = mat[2][1];
    m3[2][2] = mat[2][2];
    quat = m3.ToQuat();
}



/* idModelFbx::fbxBaseNode */
void idModelFbx::fbxBaseNode::Connect(fbxBaseNode *node, const char *prop) 
{
    fbxConnectionNode n;
    n.property = prop;

    n.node = node;
    connections.Append(n);

    n.node = this;
    node->parents.Append(n);
}

bool idModelFbx::fbxBaseNode::Convert(const fbxNode_t &object)
{
    int64_t i = GetProperty<int64_t>(object, 0, 0);
    if(!i)
        return false;
    id = i;
    const char *n = GetProperty<const char *>(object, 1, "");
	if(n) // format is "Type::name" in ASCII
	{
		const char *rn = strstr(n, "::");
		if(rn)
			n = rn + 2;
		name = n;
	}

    return true;
}

void idModelFbx::fbxBaseNode::Clear(void)
{
	id = 0;
	name.Clear();
}

const char * idModelFbx::fbxBaseNode::TypeName(int type)
{
#define _NODE_CASE(x) case x: return #x
    switch (type) {
        _NODE_CASE(GEOM);
        _NODE_CASE(MODEL);
        _NODE_CASE(MATERIAL);
        _NODE_CASE(LIMB);
        _NODE_CASE(CLUSTER);
        _NODE_CASE(SKIN);
        _NODE_CASE(CURVE);
        _NODE_CASE(XFORM);
        _NODE_CASE(ANIMLAYER);
        _NODE_CASE(ANIMSTACK);
        _NODE_CASE(MODELNULL);
        _NODE_CASE(BINDPOSE);
        default:
            return "<UNKNOWN>";
    }
#undef _NODE_CASE
}



/* idModelFbx::fbxGeometry */
bool idModelFbx::fbxGeometry::Convert(const fbxNode_t &geomNode)
{
    if(!geomNode.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(geomNode))
        return false;

    for (int i = 0; i < geomNode.elem_subtree->Num(); i++) {
        const fbxNode_t &node = geomNode.elem_subtree->operator[](i);
		if(!node.elem_props.Num())
			continue;
        if(!idStr::Icmp("Vertices", node.elem_id))
        {
			CopyArrayToList<float>(Vertices, node.elem_props[0]);
        }
        else if(!idStr::Icmp("PolygonVertexIndex", node.elem_id))
        {
			CopyArrayToList<int>(PolygonVertexIndex, node.elem_props[0]);
        }
        else if(!idStr::Icmp("Edges", node.elem_id))
        {
			CopyArrayToList<int>(Edges, node.elem_props[0]);
        }
        else if(!idStr::Icmp("LayerElementNormal", node.elem_id))
        {
            int index = this->LayerElementNormal.Append(fbxLayerElement_TEMPLATE_TYPE(Normal)());
            fbxLayerElement_TEMPLATE_TYPE(Normal) &LayerElementNormal = this->LayerElementNormal[index];

            LayerElementNormal.id = GetProperty(node, 0, 0);
            if(node.elem_subtree)
            {
                for (int m = 0; m < node.elem_subtree->Num(); m++) {
                    const fbxNode_t &n = node.elem_subtree->operator[](m);
                    if(!n.elem_props.Num())
                        continue;

                    if(!idStr::Icmp("MappingInformationType", n.elem_id))
                    {
                        LayerElementNormal.MappingInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("ReferenceInformationType", n.elem_id))
                    {
                        LayerElementNormal.ReferenceInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("Normals", n.elem_id))
                    {
                        CopyArrayToList<float>(LayerElementNormal.Normals, n.elem_props[0]);
                    }
                }
            }
        }
        else if(!idStr::Icmp("LayerElementColor", node.elem_id))
        {
            int index = this->LayerElementColor.Append(fbxLayerElement_TEMPLATE_TYPE(Color)());
            fbxLayerElement_TEMPLATE_TYPE(Color) &LayerElementColor = this->LayerElementColor[index];

            LayerElementColor.id = GetProperty(node, 0, 0);
            if(node.elem_subtree)
            {
                for (int m = 0; m < node.elem_subtree->Num(); m++) {
                    const fbxNode_t &n = node.elem_subtree->operator[](m);
                    if(!n.elem_props.Num())
                        continue;

                    if(!idStr::Icmp("MappingInformationType", n.elem_id))
                    {
                        LayerElementColor.MappingInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("ReferenceInformationType", n.elem_id))
                    {
                        LayerElementColor.ReferenceInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("Colors", n.elem_id))
                    {
                        CopyArrayToList<float>(LayerElementColor.Colors, n.elem_props[0]);
                    }
                    else if(!idStr::Icmp("ColorIndex", n.elem_id))
                    {
                        CopyArrayToList<int>(LayerElementColor.ColorIndex, n.elem_props[0]);
                    }
                }
            }
        }
        else if(!idStr::Icmp("LayerElementUV", node.elem_id))
        {
            int index = this->LayerElementUV.Append(fbxLayerElement_TEMPLATE_TYPE(UV)());
            fbxLayerElement_TEMPLATE_TYPE(UV) &LayerElementUV = this->LayerElementUV[index];

            LayerElementUV.id = GetProperty(node, 0, 0);
            if(node.elem_subtree)
            {
                for (int m = 0; m < node.elem_subtree->Num(); m++) {
                    const fbxNode_t &n = node.elem_subtree->operator[](m);
                    if(!n.elem_props.Num())
                        continue;

                    if(!idStr::Icmp("MappingInformationType", n.elem_id))
                    {
                        LayerElementUV.MappingInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("ReferenceInformationType", n.elem_id))
                    {
                        LayerElementUV.ReferenceInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("UV", n.elem_id))
                    {
                        CopyArrayToList<float>(LayerElementUV.UVs, n.elem_props[0]);
                    }
                    else if(!idStr::Icmp("UVIndex", n.elem_id))
                    {
                        CopyArrayToList<int>(LayerElementUV.UVIndex, n.elem_props[0]);
                    }
                }
            }
        }
        else if(!idStr::Icmp("LayerElementMaterial", node.elem_id))
        {
            int index = this->LayerElementMaterial.Append(fbxLayerElement_TEMPLATE_TYPE(Material)());
            fbxLayerElement_TEMPLATE_TYPE(Material) &LayerElementMaterial = this->LayerElementMaterial[index];

            LayerElementMaterial.id = GetProperty(node, 0, 0);
            if(node.elem_subtree)
            {
                for (int m = 0; m < node.elem_subtree->Num(); m++) {
                    const fbxNode_t &n = node.elem_subtree->operator[](m);
                    if(!n.elem_props.Num())
                        continue;

                    if(!idStr::Icmp("LayerElementMaterial", n.elem_id))
                    {
                        LayerElementMaterial.MappingInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("ReferenceInformationType", n.elem_id))
                    {
                        LayerElementMaterial.ReferenceInformationType = GetProperty<const char *>(n, 0, "");
                    }
                    else if(!idStr::Icmp("Materials", n.elem_id))
                    {
                        CopyArrayToList<int>(LayerElementMaterial.Materials, n.elem_props[0]);
                    }
                }
            }
        }
        else if(!idStr::Icmp("Layer", node.elem_id))
        {
            if(Layer.id != -1)
                continue;

            Layer.id = GetProperty(node, 0, 0);
            if(node.elem_subtree)
            {
                for (int m = 0; m < node.elem_subtree->Num(); m++) {
                    const fbxNode_t &n = node.elem_subtree->operator[](m);
                    if(!n.elem_subtree)
                        continue;

                    if(idStr::Icmp("LayerElement", n.elem_id))
                        continue;

                    if(!n.elem_subtree)
                        continue;

                    fbxLayerElement le;
                    for (int o = 0; o < n.elem_subtree->Num(); o++) {
                        const fbxNode_t &e = n.elem_subtree->operator[](o);

                        if(!idStr::Icmp("Type", e.elem_id))
                            le.Type = GetProperty<const char *>(e, 0, "");
                        else if(!idStr::Icmp("TypedIndex", e.elem_id))
                            le.TypedIndex = GetProperty<unsigned int>(n, 0, 0);
                    }
                    if(!le.Type.IsEmpty())
                        Layer.elements.Append(le);
                }
            }
        }
    }

    return true;
}

void idModelFbx::fbxGeometry::Clear(void)
{
	fbxBaseNode::Clear();

    Layer.id = -1;
    PolygonVertexIndex.Clear();
    Edges.Clear();
    LayerElementNormal.Clear();
    LayerElementColor.Clear();
    LayerElementUV.Clear();
    LayerElementMaterial.Clear();
}



/* idModelFbx::fbxBaseTransformNode */
bool idModelFbx::fbxBaseTransformNode::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(object))
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
        if (idStr::Icmp("Properties70", node.elem_id))
            continue;
        if(!node.elem_subtree)
            continue;

        for (int m = 0; m < node.elem_subtree->Num(); m++)
        {
            const fbxNode_t &n = node.elem_subtree->operator[](m);
            if(!n.elem_props.Num())
                continue;

            const char *type = GetProperty<const char *>(n, 1, "");
            if (!idStr::Icmp("Lcl Translation", type))
            {
                Lcl_Translation[0] = GetProperty<float>(n, 4, 0.0f);
                Lcl_Translation[1] = GetProperty<float>(n, 5, 0.0f);
                Lcl_Translation[2] = GetProperty<float>(n, 6, 0.0f);
            }
            else if (!idStr::Icmp("Lcl Rotation", type))
            {
                Lcl_Rotation[0] = GetProperty<float>(n, 4, 0.0f);
                Lcl_Rotation[1] = GetProperty<float>(n, 5, 0.0f);
                Lcl_Rotation[2] = GetProperty<float>(n, 6, 0.0f);
            }
            else if (!idStr::Icmp("Lcl Scaling", type))
            {
                Lcl_Scaling[0] = GetProperty<float>(n, 4, 1.0f);
                Lcl_Scaling[1] = GetProperty<float>(n, 5, 1.0f);
                Lcl_Scaling[2] = GetProperty<float>(n, 6, 1.0f);
            }
        }
    }

    return true;
}

void idModelFbx::fbxBaseTransformNode::Clear(void)
{
	fbxBaseNode::Clear();
	Lcl_Translation[0] = Lcl_Translation[1] = Lcl_Translation[2] = 0.0f;
	Lcl_Rotation[0] = Lcl_Rotation[1] = Lcl_Rotation[2] = 0.0f;
	Lcl_Scaling[0] = Lcl_Scaling[1] = Lcl_Scaling[2] = 1.0f;
}



/* idModelFbx::fbxCluster */
bool idModelFbx::fbxCluster::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(object))
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
		if(!node.elem_props.Num())
			continue;
        if (!idStr::Icmp("Indexes", node.elem_id))
        {
			CopyArrayToList<int>(Indexes, node.elem_props[0]);
        }
        else if (!idStr::Icmp("Weights", node.elem_id))
        {
			CopyArrayToList<float>(Weights, node.elem_props[0]);
        }
        else if (!idStr::Icmp("Transform", node.elem_id))
        {
			CopyArrayToList<float>(Transform, node.elem_props[0]);
        }
        else if (!idStr::Icmp("TransformLink", node.elem_id))
        {
			CopyArrayToList<float>(TransformLink, node.elem_props[0]);
        }
    }

    return true;
}



/* idModelFbx::fbxPoseNode */
bool idModelFbx::fbxPoseNode::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
		if(!node.elem_props.Num())
			continue;
        if (!idStr::Icmp("Node", node.elem_id))
        {
			Node = GetProperty<int64_t>(node, 0, 0);
        }
        else if (!idStr::Icmp("Matrix", node.elem_id))
        {
			CopyArrayToArray<float>(Matrix.ToFloatPtr(), 16, node.elem_props[0]);
        }
    }

    return Node != 0;
}



/* idModelFbx::fbxBindPose */
bool idModelFbx::fbxBindPose::Convert(const fbxNode_t &node)
{
    if(node.elem_props.Num() < 3)
        return false;

    if(!fbxBaseNode::Convert(node))
        return false;

    const char *type = GetProperty<const char *>(node, 2, "");
    if(!idStr::Icmp("BindPose", type))
    {
        if(node.elem_subtree)
        {
            for(int j = 0; j < node.elem_subtree->Num(); j++)
            {
                const fbxNode_t &n = node.elem_subtree->operator[](j);
                fbxPoseNode poseNode;
                if(poseNode.Convert(n))
                    PoseNode.Append(poseNode);
            }
        }
    }

    return true;
}

void idModelFbx::fbxBindPose::Clear(void)
{
    PoseNode.Clear();
}



/* idModelFbx::fbxAnimationLayer */
const char * idModelFbx::fbxAnimationLayer::AnimName(void) const
{
    int index = name.Find('|');
    return index != -1 ? name.c_str() + index + 1 : name.c_str();
}



/* idModelFbx::fbxAnimationStack */
bool idModelFbx::fbxAnimationStack::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(object))
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
        if (idStr::Icmp("Properties70", node.elem_id))
            continue;
        if(!node.elem_subtree)
            continue;

        for (int m = 0; m < node.elem_subtree->Num(); m++)
        {
            const fbxNode_t &n = node.elem_subtree->operator[](m);
            if(!n.elem_props.Num())
                continue;

            const char *type = GetProperty<const char *>(n, 0, "");
            if (!idStr::Icmp("LocalStop", type))
            {
                LocalStop = GetProperty<int64_t>(n, 4, 0);
            }
            else if (!idStr::Icmp("ReferenceStop", type))
            {
                ReferenceStop = GetProperty<int64_t>(n, 4, 0);
            }
        }

        break;
    }

    return true;
}

const char * idModelFbx::fbxAnimationStack::AnimName(void) const
{
    int index = name.Find('|');
    return index != -1 ? name.c_str() + index + 1 : name.c_str();
}

float idModelFbx::fbxAnimationStack::Seconds(void) const
{
	return (float)((double)LocalStop / (double)FBX_SEC);
}

int idModelFbx::fbxAnimationStack::NumFrames(int framerate) const
{
	return (int)((float)framerate * Seconds());
}



/* idModelFbx::fbxAnimationCurveNode */
bool idModelFbx::fbxAnimationCurveNode::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(object))
        return false;

    if(name.Length())
    {
        type = idStr::ToUpper(name[name.Length() - 1]);
    }

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
        if (idStr::Icmp("Properties70", node.elem_id))
            continue;
        if(!node.elem_subtree)
            continue;

        for (int m = 0; m < node.elem_subtree->Num(); m++)
        {
            const fbxNode_t &n = node.elem_subtree->operator[](m);
			if (idStr::Icmp("P", n.elem_id))
				continue;
            if(!n.elem_props.Num())
                continue;

            const char *comp = GetProperty<const char *>(n, 0, "");
            if (!idStr::Icmp("d|X", comp))
            {
                x = GetProperty<float>(n, 4, 0.0f);
            }
            else if (!idStr::Icmp("d|Y", comp))
            {
                y = GetProperty<float>(n, 4, 0.0f);
            }
            else if (!idStr::Icmp("d|Z", comp))
            {
                z = GetProperty<float>(n, 4, 0.0f);
            }
        }

        break;
    }

    return true;
}

const idModelFbx::fbxAnimationCurve * idModelFbx::fbxAnimationCurveNode::AnimationCurve(int index) const
{
	switch(index) {
		case 0: return FindConnection<fbxAnimationCurve>(CURVE, "d|X");
		case 1: return FindConnection<fbxAnimationCurve>(CURVE, "d|Y");
		case 2: return FindConnection<fbxAnimationCurve>(CURVE, "d|Z");
		default: return NULL;
	}
}



/* idModelFbx::fbxAnimationCurve */
bool idModelFbx::fbxAnimationCurve::Convert(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    if(!fbxBaseNode::Convert(object))
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
		if(!node.elem_props.Num())
			continue;

		if (!idStr::Icmp("KeyTime", node.elem_id))
		{
			CopyArrayToList<int64_t>(KeyTime, node.elem_props[0]);
		}
		else if (!idStr::Icmp("KeyValueFloat", node.elem_id))
		{
			CopyArrayToList<float>(KeyValueFloat, node.elem_props[0]);
		}
		else if (!idStr::Icmp("Default", node.elem_id))
        {
			Default = GetProperty<float>(node, 0, 0.0f);
        }
    }

    return true;
}

float idModelFbx::fbxAnimationCurve::LinearData(int64_t time) const
{
	if(KeyTime.Num() == 0 || KeyValueFloat.Num() == 0)
		return Default;
	if(time <= KeyTime[0])
		return KeyValueFloat[0];
	else if(time >= KeyTime[KeyTime.Num() - 1])
		return KeyValueFloat[KeyValueFloat.Num() - 1];

	for(int i = 0; i < KeyTime.Num() - 1; i++)
	{
		if(time >= KeyTime[i] && time <= KeyTime[i + 1])
		{
			return (float)(
					((double)(KeyValueFloat[i + 1] - KeyValueFloat[i]) 
					* (double)(time - KeyTime[i]) / (double)(KeyTime[i + 1] - KeyTime[i]))
					+ (double)KeyValueFloat[i]
			); // always linear
		}
	}
	return Default;
}

void idModelFbx::fbxAnimationCurve::GenFrameData(idList<float> &frames, int numFrames) const
{
	assert(KeyTime.Num() > 0 && KeyTime[0] == 0);
	assert(KeyTime.Num() == KeyValueFloat.Num());

	frames.SetNum(numFrames);
	double unit = (double)(KeyTime[KeyTime.Num() - 1] - KeyTime[0]) / (double)numFrames;
	for(int i = 0; i < numFrames; i++)
	{
		frames[i] = LinearData((int64_t)(unit * i));
	}
}



/* idModelFbx::fbxConnection */
bool idModelFbx::fbxConnection::Convert(const fbxNode_t &node)
{
    if(idStr::Icmp("C", node.elem_id))
        return false;

    type = GetProperty<const char *>(node, 0, "");
    from = GetProperty<int64_t>(node, 1, 0);
    to = GetProperty<int64_t>(node, 2, 0);
	if(idStr::Icmp("OP", type))
		property = GetProperty<const char *>(node, 3, "");

    return true;
}



/* idModelFbx::fbxObject */
bool idModelFbx::fbxObject::Convert(const fbxNode_t &objectsNode)
{
    if(!objectsNode.elem_subtree)
        return false;

    for (int i = 0; i < objectsNode.elem_subtree->Num(); i++) {
        const fbxNode_t &node = objectsNode.elem_subtree->operator[](i);
        if(!idStr::Icmp("Geometry", node.elem_id))
        {
            Geometry.Convert(node);
        }
        else if(!idStr::Icmp("Material", node.elem_id))
        {
            fbxMaterial mat;
            if(mat.Convert(node))
                Material.Append(mat);
        }
        else if(!idStr::Icmp("Model", node.elem_id))
        {
            const char *type = GetProperty<const char *>(node, 2, "");
            if(!idStr::Icmp("LimbNode", type))
            {
                fbxLimbNode limb;
                if(limb.Convert(node))
                    LimbNode.Append(limb);
            }
            else if(!idStr::Icmp("Mesh", type))
            {
                fbxMesh mesh;
                if(mesh.Convert(node))
                    Mesh = mesh;
            }
            else if(!idStr::Icmp("Null", type))
            {
                fbxNull mnull;
                if(mnull.Convert(node))
                    Null = mnull;
            }
        }
        else if(!idStr::Icmp("Deformer", node.elem_id))
        {
            const char *type = GetProperty<const char *>(node, 2, "");
            if(!idStr::Icmp("Cluster", type))
            {
                fbxCluster cluster;
                if(cluster.Convert(node))
                    Cluster.Append(cluster);
            }
			else if(!idStr::Icmp("Skin", type))
            {
                fbxSkin skin;
                if(skin.Convert(node))
                    Skin = skin;
            }
        }
        else if(!idStr::Icmp("Pose", node.elem_id))
        {
            fbxBindPose pose;
            if(pose.Convert(node))
                BindPose = pose;
        }
        else if(!idStr::Icmp("AnimationLayer", node.elem_id))
        {
            fbxAnimationLayer animLayer;
            if(animLayer.Convert(node))
                AnimationLayer.Append(animLayer);
        }
        else if(!idStr::Icmp("AnimationStack", node.elem_id))
        {
            fbxAnimationStack animStack;
            if(animStack.Convert(node))
                AnimationStack.Append(animStack);
        }
        else if(!idStr::Icmp("AnimationCurve", node.elem_id))
        {
            fbxAnimationCurve animCurve;
            if(animCurve.Convert(node))
                AnimationCurve.Append(animCurve);
        }
        else if(!idStr::Icmp("AnimationCurveNode", node.elem_id))
        {
            fbxAnimationCurveNode curveNode;
            if(curveNode.Convert(node))
                AnimationCurveNode.Append(curveNode);
        }
    }

    return true;
}

void idModelFbx::fbxObject::Clear(void)
{
	Skin.Clear();
    Geometry.Clear();
    Mesh.Clear();
    LimbNode.Clear();
    Cluster.Clear();
    Material.Clear();
    BindPose.Clear();

    AnimationLayer.Clear();
    AnimationStack.Clear();
    AnimationCurveNode.Clear();
    AnimationCurve.Clear();
}

int idModelFbx::fbxObject::AnimCount(void) const
{
    return AnimationStack.Num();
}

const char * idModelFbx::fbxObject::AnimName(unsigned int index) const
{
    if(index >= (unsigned int)AnimCount())
        return NULL;
    return AnimationStack[index].AnimName();
}



/* idModelFbx::fbxModel */
idModelFbx::fbxBaseNode * idModelFbx::fbxModel::FindNode(int64_t id) {
    for(int i = 0; i < nodes.Num(); i++)
    {
        if(nodes[i]->id == id)
        {
            return nodes[i];
        }
    }
    return NULL;
}

const idModelFbx::fbxBaseNode * idModelFbx::fbxModel::FindNode(int64_t id) const
{
    for(int i = 0; i < nodes.Num(); i++)
    {
        if(nodes[i]->id == id)
        {
            return nodes[i];
        }
    }
    return NULL;
}

void idModelFbx::fbxModel::Clear(void)
{
    Objects.Clear();
    Connections.Clear();
    nodes.Clear();
    root.Clear();
}

bool idModelFbx::fbxModel::ToConnections(const fbxNode_t &object)
{
    if(!object.elem_subtree)
        return false;

    for (int i = 0; i < object.elem_subtree->Num(); i++)
    {
        const fbxNode_t &node = object.elem_subtree->operator[](i);
        if(idStr::Icmp("C", node.elem_id))
            continue;

        const char *type = GetProperty<const char *>(node, 0, "");
        if(idStr::Icmp("OO", type) && idStr::Icmp("OP", type))
            continue;

        fbxConnection conn;
        if(conn.Convert(node))
            Connections.Append(conn);
    }

    return true;
}

bool idModelFbx::fbxModel::Convert(const idList<fbxNode_t> &rootNodes)
{
    if(rootNodes.Num() == 0)
        return false;

    for (int i = 0; i < rootNodes.Num(); i++) {
        const fbxNode_t &node = rootNodes[i];
        if(!idStr::Icmp("Objects", node.elem_id))
        {
            fbxObject &object = Objects;
            if(!object.Convert(node))
            {
                common->Warning("Convert fbx 'Objects' error");
                return false;
            }
        }
        else if(!idStr::Icmp("Connections", node.elem_id))
        {
            if(!ToConnections(node))
            {
                common->Warning("Convert fbx 'Connections' error");
                return false;
            }
        }
    }

    AddNodes();
    AddConnections();

    return true;
}

void idModelFbx::fbxModel::AddConnections(void)
{
    for(int i = 0; i < Connections.Num(); i++)
    {
        const fbxConnection &conn = Connections[i];
        fbxBaseNode *from = FindNode(conn.from);
        if(!from)
            continue;

        if(conn.to == 0)
        {
            fbxConnectionNode n;
            n.property = conn.property.c_str();
            n.node = from;
            root.Append(n);
        }
        else
        {
            fbxBaseNode *to = FindNode(conn.to);
            if(to)
                to->Connect(from, conn.property.c_str());
        }
    }
}

int idModelFbx::fbxModel::AddNodes(void)
{
#define _ADD_NODES(L) \
	for(int m = 0; m < L.Num(); m++) { \
		nodes.Append(&L[m]); \
	}

    nodes.Clear();

    nodes.Append(&Objects.Geometry);
    nodes.Append(&Objects.Mesh);
    nodes.Append(&Objects.Skin);
    nodes.Append(&Objects.Null);
    nodes.Append(&Objects.BindPose);
    _ADD_NODES(Objects.LimbNode);
    _ADD_NODES(Objects.Cluster);
    _ADD_NODES(Objects.Material);

    _ADD_NODES(Objects.AnimationLayer);
    _ADD_NODES(Objects.AnimationStack);
    _ADD_NODES(Objects.AnimationCurveNode);
    _ADD_NODES(Objects.AnimationCurve);

    return nodes.Num();

#undef _ADD_NODES
}

void idModelFbx::fbxModel::Print(void) const
{
#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
    if(all) { \
        for(int i = 0; i < list.Num(); i++) { \
             Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__); \
        } \
    } \
    Sys_Printf("\n------------------------------------------------------\n");

    const fbxBaseNode *a, *b;

    Sys_Printf("Geometry: %s, id=%lld, vertex=%d, index=%d, edge=%d, normal=%d, color=%d, uv=%d, material=%d, layer=%d\n", Objects.Geometry.name.c_str(), (long long)Objects.Geometry.id, Objects.Geometry.Vertices.Num(), Objects.Geometry.PolygonVertexIndex.Num(), Objects.Geometry.Edges.Num(), Objects.Geometry.LayerElementNormal.Num(), Objects.Geometry.LayerElementColor.Num(), Objects.Geometry.LayerElementUV.Num(), Objects.Geometry.LayerElementMaterial.Num(), Objects.Geometry.Layer.id);
    MODEL_PART_PRINT(Layer, Objects.Geometry.Layer.elements, true, "%s, id=%d   ", Objects.Geometry.Layer.elements[i].Type.c_str(), Objects.Geometry.Layer.elements[i].TypedIndex)

    Sys_Printf("Mesh: %s, id=%lld, Translation=(%f, %f, %f), Rotation=(%f, %f, %f), Scale=(%f, %f, %f)\n", Objects.Mesh.name.c_str(), (long long)Objects.Mesh.id, Objects.Mesh.Lcl_Translation[0], Objects.Mesh.Lcl_Translation[1], Objects.Mesh.Lcl_Translation[2], Objects.Mesh.Lcl_Rotation[0], Objects.Mesh.Lcl_Rotation[1], Objects.Mesh.Lcl_Rotation[2], Objects.Mesh.Lcl_Scaling[0], Objects.Mesh.Lcl_Scaling[1], Objects.Mesh.Lcl_Scaling[2]);
    Sys_Printf("Skin: %s, id=%lld\n", Objects.Skin.name.c_str(), (long long)Objects.Skin.id);

    MODEL_PART_PRINT(Limb, Objects.LimbNode, true, "%s, Translation=(%f, %f, %f), Rotation=(%f, %f, %f)   ", Objects.LimbNode[i].name.c_str(), Objects.LimbNode[i].Lcl_Translation[0], Objects.LimbNode[i].Lcl_Translation[1], Objects.LimbNode[i].Lcl_Translation[2], Objects.LimbNode[i].Lcl_Rotation[0], Objects.LimbNode[i].Lcl_Rotation[1], Objects.LimbNode[i].Lcl_Rotation[2])
    MODEL_PART_PRINT(Cluster, Objects.Cluster, true, "%s, index=%d, weight=%d, transform=%d, transformLink=%d   ", Objects.Cluster[i].name.c_str(), Objects.Cluster[i].Indexes.Num(), Objects.Cluster[i].Weights.Num(), Objects.Cluster[i].Transform.Num(), Objects.Cluster[i].TransformLink.Num())
    MODEL_PART_PRINT(Material, Objects.Material, true, "%s   ", Objects.Material[i].name.c_str())
    MODEL_PART_PRINT(BindPose, Objects.BindPose.PoseNode, true, "%s, node=%lld   ", (a = FindNode(Objects.BindPose.PoseNode[i].Node)) ? a->name.c_str() : "<NULL>", (long long)Objects.BindPose.PoseNode[i].Node)

    MODEL_PART_PRINT(AnimationLayer, Objects.AnimationLayer, true, "%s, anim=%s   ", Objects.AnimationLayer[i].name.c_str(), Objects.AnimationLayer[i].AnimName())
    MODEL_PART_PRINT(AnimationStack, Objects.AnimationStack, true, "%s, anim=%s, LocalStop=%lld,ReferenceStop=%lld    ", Objects.AnimationStack[i].name.c_str(), Objects.AnimationStack[i].AnimName(), (long long)Objects.AnimationStack[i].LocalStop, (long long)Objects.AnimationStack[i].ReferenceStop)
    MODEL_PART_PRINT(AnimationCurveNode, Objects.AnimationCurveNode, true, "%s, default=(%f, %f, %f)    ", Objects.AnimationCurveNode[i].name.c_str(), Objects.AnimationCurveNode[i].x, Objects.AnimationCurveNode[i].y, Objects.AnimationCurveNode[i].z)
    MODEL_PART_PRINT(fbxAnimationCurve, Objects.AnimationCurve, true, "%s, default=%f, times=%d, keys=%d    ", Objects.AnimationCurve[i].name.c_str(), Objects.AnimationCurve[i].Default, Objects.AnimationCurve[i].KeyTime.Num(), Objects.AnimationCurve[i].KeyValueFloat.Num())

    MODEL_PART_PRINT(nodes, nodes, false, "   %s", "")
    MODEL_PART_PRINT(root, root, true, "id=%lld, name=%s, type=%s   ", (long long)root[i].node->id, root[i].node->name.c_str(), fbxBaseNode::TypeName(root[i].node->Type()))
    MODEL_PART_PRINT(Connections, Connections, true, "type=%s, property=%s, from=%lld(%s), to=%lld(%s)   ", Connections[i].type.c_str(), Connections[i].property.c_str(), (long long)Connections[i].from, (a = FindNode(Connections[i].from)) ? va("%s::%s", fbxBaseNode::TypeName(a->Type()), a->name.c_str()) : "<NULL>", (long long)Connections[i].to, (b = FindNode(Connections[i].to)) ? va("%s::%s", fbxBaseNode::TypeName(b->Type()), b->name.c_str()) : "<NULL>")
#undef MODEL_PART_PRINT
}



/* idModelFbx */
idModelFbx::idModelFbx(void)
        : file(NULL),
		  lexer(NULL),
          fbx_version(-1),
          types(0)
{}

idModelFbx::~idModelFbx(void)
{
    if(file)
        fileSystem->CloseFile(file);
}

void idModelFbx::Clear(void)
{
    model.Clear();
    if(file)
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }
    if(lexer)
    {
        lexer = NULL;
    }
    fbx_version = -1;
}

void idModelFbx::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelFbx::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

int idModelFbx::ReadHeader(fbxHeader_t &header)
{
    if(file->Length() < (int)sizeof(fbxHeader_t))
    {
        common->Warning("Unexpected end of file(%d < %zu bytes).", file->Length(), sizeof(header.magic));
        return -1;
    }

    memset(&header, 0, sizeof(header));

    file->Read(header.magic, sizeof(header.magic));

    const char _HEAD_MAGIC[] = FBX_MAGIC;
    if(memcmp(header.magic, _HEAD_MAGIC, sizeof(_HEAD_MAGIC)) != 0)
    {
        common->Warning("Invalid header");
        return -1;
    }

    file->ReadUnsignedInt(header.fbx_version);

    return sizeof(header);
}

void * idModelFbx::AllocArrayData(fbxArray_t &data, unsigned int length)
{
    data.length = length;
    data.ptr = calloc(length, 1);
    data.count = length;
    data.stride = 1;
    return data.ptr;
}

void idModelFbx::FreeArrayData(fbxArray_t &data)
{
    data.length = 0;
    if(data.ptr)
    {
        free(data.ptr);
        data.ptr = NULL;
    }
    data.stride = 0;
    data.count = 0;
}

void idModelFbx::FreeProperty(fbxProperty_t &prop)
{
    if (prop.type == FBX_ARRAY_BOOL
        || prop.type == FBX_ARRAY_BYTE
        || prop.type == FBX_ARRAY_INT32
        || prop.type == FBX_ARRAY_INT64
        || prop.type == FBX_ARRAY_FLOAT32
        || prop.type == FBX_ARRAY_FLOAT64
        || prop.type == FBX_DATA_BINARY
        || prop.type == FBX_DATA_STRING
    )
        FreeArrayData(prop.data.a);
}

void idModelFbx::FreeObject(fbxNode_t &object)
{
    for(int i = 0; i < object.elem_props.Num(); i++)
    {
        FreeProperty(object.elem_props[i]);
    }
    if(object.elem_subtree)
    {
        for(int i = 0; i < object.elem_subtree->Num(); i++)
        {
            FreeObject(object.elem_subtree->operator[](i));
        }
        delete object.elem_subtree;
        object.elem_subtree = NULL;
    }
}

int idModelFbx::read_elem_start64(fbxElem64_t &elem)
{
    if(file->Length() - file->Tell() < 25)
        return 0;

    int res = 0;
    res += ReadUnsignedLongLong(file, elem.end_offset);
    res += ReadUnsignedLongLong(file, elem.prop_count);
    res += ReadUnsignedLongLong(file, elem._prop_length);
    res += file->ReadUnsignedChar(elem.elem_id_size);
    if(elem.elem_id_size)
    {
        char *arr = (char *)malloc(elem.elem_id_size);
        res += file->Read(arr, elem.elem_id_size);
        elem.elem_id.Append(arr, elem.elem_id_size);
    }
    else
        elem.elem_id.Clear();
    return res;
}

int idModelFbx::read_elem_start32(fbxElem32_t &elem)
{
    if(file->Length() - file->Tell() < 13)
        return 0;

    int res = 0;
    res += file->ReadUnsignedInt(elem.end_offset);
    res += file->ReadUnsignedInt(elem.prop_count);
    res += file->ReadUnsignedInt(elem._prop_length);
    res += file->ReadUnsignedChar(elem.elem_id_size);
    if(elem.elem_id_size)
    {
        char *arr = (char *)malloc(elem.elem_id_size);
        res += file->Read(arr, elem.elem_id_size);
        elem.elem_id.Append(arr, elem.elem_id_size);
    }
    else
        elem.elem_id.Clear();
    return res;
}

uint64_t idModelFbx::read_fbx_elem_start(uint64_t &end_offset, uint64_t &prop_count, idStr &elem_id)
{
    if(fbx_version < 7500)
    {
        fbxElem32_t elem;
        if(!read_elem_start32(elem))
            return 0;
        end_offset = elem.end_offset;
        prop_count = elem.prop_count;
        elem_id = elem.elem_id;
    }
    else
    {
        fbxElem64_t elem;
        if(!read_elem_start64(elem))
            return 0;
        end_offset = elem.end_offset;
        prop_count = elem.prop_count;
        elem_id = elem.elem_id;
    }
    return end_offset;
}

int idModelFbx::read_array_params(fbxArrayParam_t &param)
{
    if(file->Length() - file->Tell() < 12)
        return 0;
    file->ReadUnsignedInt(param.length);
    file->ReadUnsignedInt(param.encoding);
    file->ReadUnsignedInt(param.comp_len);
    return 12;
}

int idModelFbx::_create_array(fbxArray_t &data, int length, int array_type, int array_stride, bool array_byteswap)
{
    if(length * array_stride != data.length)
    {
        common->Warning("Array length not match: %u != %u", length * array_stride, data.length);
        return -1;
    }

    data.stride = array_stride;
    data.count = length;
    if(array_byteswap && Swap_IsBigEndian())
    {
        for(int i = 0; i < length; i++)
        {
            if (array_stride == 8) {
                int64_t *ptr64 = ((int64_t *)data.ptr) + i;
                *ptr64 = LongLongSwap(*ptr64);
            }
            else
            {
                int32_t *ptr32 = ((int32_t *)data.ptr) + i;
                *ptr32 = LongSwap(*ptr32);
            }
        }
    }

    return length;
}

int idModelFbx::_decompress_and_insert_array(fbxArray_t &ret, const fbxArray_t &compressed_data, int length, int array_type, int array_stride, bool array_byteswap)
{
    int decomp_length;
    byte *data = zlib_decompress((byte *)compressed_data.ptr, compressed_data.length, &decomp_length);
    if(!data)
    {
        common->Warning("Decompressed array error: %d", decomp_length);
        return -1;
    }

    if(length * array_stride != decomp_length)
    {
        free(data);
        common->Warning("Decompressed array length not match: %u != %d", length * array_stride, decomp_length);
        return -1;
    }
    ret.length = decomp_length;
    ret.ptr = data;

    int res = _create_array(ret, length, array_type, array_stride, array_byteswap);
    if(res < 0)
        FreeArrayData(ret);
    return res;
}

int idModelFbx::unpack_array(fbxArray_t &data, int array_type, int array_stride, bool array_byteswap)
{
    fbxArrayParam_t arrayParam;
    read_array_params(arrayParam);

    if(arrayParam.encoding == 1)
    {
        fbxArray_t comp_data;
        AllocArrayData(comp_data, arrayParam.comp_len);
        file->Read(comp_data.ptr, arrayParam.comp_len);
        int ret = _decompress_and_insert_array(data, comp_data, arrayParam.length, array_type, array_stride, array_byteswap);
        FreeArrayData(comp_data);
        if(ret < 0)
            FreeArrayData(data);
        return ret;
    }
    else
    {
        AllocArrayData(data, arrayParam.comp_len);
        file->Read(data.ptr, arrayParam.comp_len);
        int res = _create_array(data, arrayParam.length, array_type, array_stride, array_byteswap);
        if(res < 0)
            FreeArrayData(data);
        return res;
    }
}

int idModelFbx::read_array_dict(fbxArray_t &data, int array_type)
{
    switch (array_type) {
        case FBX_ARRAY_BOOL:
            return unpack_array(data, array_type, 1, false);
        case FBX_ARRAY_BYTE:
            return unpack_array(data, array_type, 1, false);
        case FBX_ARRAY_INT32:
            return unpack_array(data, array_type, 4, true);
        case FBX_ARRAY_INT64:
            return unpack_array(data, array_type, 8, true);
        case FBX_ARRAY_FLOAT32:
            return unpack_array(data, array_type, 4, false);
        case FBX_ARRAY_FLOAT64:
            return unpack_array(data, array_type, 8, false);
        default:
            //common->Warning("Unknown array type: %d", array_type);
            return 0;
    }
}

int idModelFbx::read_data_dict(fbxData_t &data, int data_type)
{
    switch (data_type) {
        case FBX_DATA_BYTE:
            return file->ReadUnsignedChar(data.b);
        case FBX_DATA_INT16:
            return file->ReadShort(data.h);
        case FBX_DATA_BOOL:
            return file->ReadBool(data.z);
        case FBX_DATA_CHAR:
            return file->ReadChar(data.c);
        case FBX_DATA_INT32:
            return file->ReadInt(data.i);
        case FBX_DATA_FLOAT32:
            return file->ReadFloat(data.f);
        case FBX_DATA_FLOAT64:
            return ReadDouble(file, data.d);
        case FBX_DATA_INT64:
            return ReadLongLong(file, data.l);
        case FBX_DATA_BINARY:
            file->ReadUnsignedInt(data.a.length);
            AllocArrayData(data.a, data.a.length);
            return file->Read(data.a.ptr, data.a.length);
        case FBX_DATA_STRING:
            file->ReadUnsignedInt(data.a.length);
            AllocArrayData(data.a, data.a.length + 1);
            data.a.length -= 1;
			((char *)data.a.ptr)[data.a.length] = '\0';
            return file->Read(data.a.ptr, data.a.length);
        default:
            common->Warning("Unknown data type: %d", data_type);
            return 0;
    }
}

int idModelFbx::read_elem(fbxNode_t &object, int tell_file_offset)
{
    uint64_t end_offset;
    uint64_t prop_count;
    idStr elem_id;

    if(!read_fbx_elem_start(end_offset, prop_count, elem_id))
        return 0;

    object.elem_id = elem_id;
    object.elem_subtree = NULL;
    object.elem_props.SetNum(prop_count);
    for (uint64_t i = 0; i < prop_count; i++) {
        fbxProperty_t &parm = object.elem_props[i];
        file->ReadUnsignedChar(parm.type);
        int ret = read_array_dict(parm.data.a, parm.type);
        if(ret < 0)
            parm.type = 0; // mark invalid
        else if(ret == 0)
            ret = read_data_dict(parm.data, parm.type);
    }

    int pos = file->Tell();
    int local_end_offset = (int)end_offset - tell_file_offset;
    const int _BLOCK_SENTINEL_LENGTH = FBX_BLOCK_SENTINEL_LENGTH();

    idList<fbxNode_t> elem_subtree;
    idFile *old_file = file;
    int start_sub_pos;
    int sub_tree_end;
    idList<char> sub_elem_bytes;
    if(pos < local_end_offset)
    {
        if (tell_file_offset == 0 && !idStr::Icmp(elem_id, "Objects"))
        {
            int block_bytes_remaining = local_end_offset - pos;

            sub_elem_bytes.SetNum(block_bytes_remaining);
            int num_bytes_read = file->Read(sub_elem_bytes.Ptr(), block_bytes_remaining);

            if (num_bytes_read != block_bytes_remaining)
            {
                common->Warning("failed to read complete nested block, expected %i bytes, but only got %i", block_bytes_remaining, num_bytes_read);
            }

            idFile_Memory *f = new idFile_Memory("Fbx::Objects::children", (const char *)sub_elem_bytes.Ptr(), sub_elem_bytes.Num());
            file = f;

            start_sub_pos = 0;
            tell_file_offset = pos;
            sub_tree_end = block_bytes_remaining - _BLOCK_SENTINEL_LENGTH;
        }
        else
        {
            start_sub_pos = pos;
            sub_tree_end = local_end_offset - _BLOCK_SENTINEL_LENGTH;
        }

        int sub_pos = start_sub_pos;
        while(sub_pos < sub_tree_end)
        {
            fbxNode_t obj;
            if(read_elem(obj, tell_file_offset))
                elem_subtree.Append(obj);
            sub_pos = file->Tell();
        }

        idList<byte> sentinel;
        sentinel.SetNum(_BLOCK_SENTINEL_LENGTH);
        int sentinel_length = file->Read(sentinel.Ptr(), _BLOCK_SENTINEL_LENGTH);
        if(sentinel_length != _BLOCK_SENTINEL_LENGTH)
        {
            common->Warning("failed to read nested block sentinel: %d != %d", sentinel_length, _BLOCK_SENTINEL_LENGTH);
        }
        bool valid = true;
        for (int i = 0; i < sentinel.Num(); i++)
        {
            if(sentinel[i] != 0)
            {
                valid = false;
                break;
            }
        }
        if(!valid)
        {
            common->Warning("failed to read nested block sentinel, "
                            "expected all bytes to be 0");
        }

        pos += (sub_pos - start_sub_pos) + _BLOCK_SENTINEL_LENGTH;
    }

    if(old_file != file)
    {
        delete file;
        file = old_file;
    }

    if(elem_subtree.Num() > 0)
    {
        object.elem_subtree = new idList<fbxNode_t>(elem_subtree);
    }

    if(pos != local_end_offset)
    {
        common->Warning("scope length not reached, something is wrong");
    }

    return pos;
}

int idModelFbx::ReadToken(idToken *token, bool onLine)
{
	int res = onLine ? lexer->ReadTokenOnLine(token) : lexer->ReadToken(token);
	//if(res) common->Printf("TK%c %d: |%s|\n", onLine ? 'L' : 'N', token->type, token->c_str());
	return res;
}

void idModelFbx::AsciiSkipComment(void)
{
	idToken token;

	while(true)
	{
		if(!ReadToken(&token))
			break;

		if(token.type == TT_PUNCTUATION && token == ";")
		{
			idStr unused;
			lexer->ReadRestOfLine(unused);
		}
		else
		{
			lexer->UnreadToken(&token);
			break;
		}
	}
}

int idModelFbx::AsciiParseNumber(int64_t &l, double &d)
{
	//common->Printf("AsciiParseNumber\n");
	idToken token;
	bool neg = false;
	bool number = false;
	idStr str;
	bool isFloat = false;
	bool e = false;
	bool eNumber = false;
	bool eNeg = false;
	bool end = false;
	bool unread = false;

	while(true)
	{
		if(!ReadToken(&token, false))
		{
			break;
		}

		//Sys_Printf("Num: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
		if(token.type == TT_NUMBER)
		{
			if(e)
			{
				if(eNumber)
				{
					end = true;
					unread = true;
				}
				else
				{
					if(token.subtype & TT_INTEGER)
					{
						if(token.GetIntValue() != 0)
						{
							eNumber = true;
							str.Append(token.c_str());
						}
						else
							continue;
					}
					else
					{
						end = true;
						unread = true;
					}
				}
			}
			else
			{
				if(number)
				{
					end = true;
					unread = true;
				}
				else
				{
					number = true;
					if(token.subtype & TT_INTEGER)
					{
						isFloat = false;
					}
					else
					{
						isFloat = true;
					}
					str.Append(token.c_str());
				}
			}
		}
		else if(token.type == TT_LITERAL || token.type == TT_NAME)
		{
			if(!idStr::Icmp(token, "e"))
			{
				if(e)
				{
					end = true;
					unread = true;
				}
				else if(number)
				{
					isFloat = true;
					e = true;
					str.Append(token.c_str());
				}
				else
				{
					end = true;
					unread = true;
				}
			}
			else
			{
				end = true;
				unread = true;
			}
		}
		else if(token.type == TT_PUNCTUATION)
		{
			if(token == "-")
			{
				if(e)
				{
					if(eNeg)
					{
						end = true;
						unread = true;
					}
					else if(!eNumber)
					{
						eNeg = true;
						str.Append(token.c_str());
					}
					else
					{
						end = true;
						unread = true;
					}
				}
				else
				{
					if(neg)
					{
						end = true;
						unread = true;
					}
					else if(!number)
					{
						neg = true;
						str.Append(token.c_str());
					}
					else
					{
						end = true;
						unread = true;
					}
				}
			}
			else
			{
				end = true;
				unread = true;
			}
		}
		else
		{
			end = true;
			unread = true;
			break;
		}

		if(end)
			break;
	}

	if(!number)
	{
		lexer->Error("Parse number: Except number value\n");
		return -1;
	}
	else if(e && !eNumber)
	{
		lexer->Error("Parse number: Except E number value\n");
		return -1;
	}

	int type = 0;
	if(isFloat)
	{
		if(e)
		{
			if(sscanf(str.c_str(), "%le", &d) == 1)
            {
				if(d > FLT_MAX || d < FLT_MIN)
					type = FBX_DATA_FLOAT64;
				else
					type = FBX_DATA_FLOAT32;
				l = (int64_t)d;
            }
			else
			{
				lexer->Error("Parse number: Parse E float number error '%s'\n", str.c_str());
				return -1;
			}
		}
		else
		{
			if(sscanf(str.c_str(), "%lf", &d) == 1)
            {
				if(d > FLT_MAX || d < FLT_MIN)
					type = FBX_DATA_FLOAT64;
				else
					type = FBX_DATA_FLOAT32;
				l = (int64_t)d;
            }
			else
			{
				lexer->Error("Parse number: Parse float number error '%s'\n", str.c_str());
				return -1;
			}
		}
	}
	else
	{
		long long lld;
		if(sscanf(str.c_str(), "%lld", &lld) == 1)
        {
			/*if(lld == 0 || lld == 1)
				type = FBX_DATA_BOOL;
			else*/ if(lld <= INT8_MAX && lld >= INT8_MIN && isprint((char)lld))
				type = FBX_DATA_CHAR;
			else if(lld <= UINT8_MAX && lld >= 0)
				type = FBX_DATA_BYTE;
			else if(lld <= INT16_MAX && lld >= INT16_MIN)
				type = FBX_DATA_INT16;
			else if(lld <= INT32_MAX && lld >= INT32_MIN)
				type = FBX_DATA_INT32;
			else if(lld <= INT64_MAX && lld >= INT64_MIN)
				type = FBX_DATA_INT64;
			else
				type = FBX_DATA_INT64;
			l = (int64_t)lld;
			d = (double)lld;
        }
		else
		{
			lexer->Error("Parse number: Parse integer number error '%s'\n", str.c_str());
			return -1;
		}
	}

	if(unread)
		lexer->UnreadToken(&token);

	return type;
}

int idModelFbx::AsciiParseArray(fbxArray_t &array)
{
	//common->Printf("AsciiParseArray\n");
	if(!lexer->ExpectTokenString("*"))
		return -1;

	int num = lexer->ParseInt();
	if(num <= 0)
		return num;

	idToken token;
	idList<int64_t> ls;
	idList<double> ds;
	idList<byte> types;

	if(!lexer->ExpectTokenString("{"))
		return -1;

	if(!ReadToken(&token))
	{
		lexer->Error("Expect literal token 'a'\n");
		return -1;
	}
	if((token.type != TT_LITERAL && token.type != TT_NAME) || token != "a")
	{
		lexer->Error("Expect literal token 'a', but found '%s'\n", token.c_str());
		return -1;
	}

	if(!ReadToken(&token))
	{
		lexer->Error("Expect literal token ':'\n");
		return -1;
	}
	if(token.type != TT_PUNCTUATION || token != ":")
	{
		lexer->Error("Expect punctuation token ':', but found '%s'\n", token.c_str());
		return -1;
	}

	ls.SetNum(num);
	ds.SetNum(num);
	for(int i = 0; i < num; i++)
	{
		int64_t l;
		double d;
		int type = AsciiParseNumber(l, d);
		if(type <= 0)
		{
			lexer->Error("Parse array number error\n");
			return -1;
		}
		ls[i] = l;
		ds[i] = d;
		types.AddUnique(type);

		if(i < num - 1)
			lexer->ExpectTokenString(",");
	}

	if(!lexer->ExpectTokenString("}"))
		return -1;

	int type = FBX_ARRAY_FLOAT64;
#define _CHECK_TYPE(s, t) if(types.FindIndex(s) != -1) type = t; // small to big
	_CHECK_TYPE(FBX_DATA_BOOL, FBX_ARRAY_BOOL)
	_CHECK_TYPE(FBX_DATA_CHAR, FBX_ARRAY_BYTE)
	_CHECK_TYPE(FBX_DATA_BYTE, FBX_ARRAY_BYTE)
	_CHECK_TYPE(FBX_DATA_INT16, FBX_ARRAY_INT32)
	_CHECK_TYPE(FBX_DATA_INT32, FBX_ARRAY_INT32)
	_CHECK_TYPE(FBX_DATA_INT64, FBX_ARRAY_INT64)
	_CHECK_TYPE(FBX_DATA_FLOAT32, FBX_ARRAY_FLOAT32)
	_CHECK_TYPE(FBX_DATA_FLOAT64, FBX_ARRAY_FLOAT64)
#undef _CHECK_TYPE

#define _COPY_ARR(T, lst) { \
	AllocArrayData(array, num * sizeof(T)); \
	array.count = num; \
	array.stride = sizeof(T); \
	T *ptr = (T *)array.ptr; \
	for(int i = 0; i < num; i++) { \
		ptr[i] = (T)lst[i]; \
	} \
}
	switch (type) {
		case FBX_ARRAY_BOOL:
			_COPY_ARR(bool, ls)
			break;
		case FBX_ARRAY_BYTE:
			_COPY_ARR(byte, ls)
			break;
		case FBX_ARRAY_INT32:
			_COPY_ARR(int, ls)
			break;
		case FBX_ARRAY_INT64:
			_COPY_ARR(int64_t, ls)
			break;
		case FBX_ARRAY_FLOAT32:
			_COPY_ARR(float, ds)
			break;
		case FBX_ARRAY_FLOAT64:
			_COPY_ARR(double, ds)
			break;
		default:
			break;
	}
#undef _COPY_ARR

	return type;
}

int idModelFbx::AsciiParseProperty(idList<fbxProperty_t> &props)
{
	//common->Printf("AsciiParseProperty\n");
	if(!lexer->ExpectTokenString(":"))
		return -1;

	idToken token;
	int num = 0;

	while(true)
	{
		AsciiSkipComment();

		if(!ReadToken(&token, true))
			break;

		bool reqNum = false;
		if(token.type == TT_PUNCTUATION)
		{
			if(token == "{") // next is children
			{
				lexer->UnreadToken(&token);
				break;
			}
			else if(token == "-") // number
			{
				lexer->UnreadToken(&token);
				reqNum = true;
			}
			else if(token == "*") // array
			{
				lexer->UnreadToken(&token);
				int index = props.Append(fbxProperty_t());
				fbxProperty_t &prop = props[index];
				prop.type = AsciiParseArray(prop.data.a);
				if(prop.type > 0)
					num++;
				else
					props.SetNum(index);
			}
			else if(token == ",") // next property
			{
				continue;
			}
			else
			{
				lexer->Error("Read unexpected punctuation token: %s\n", token.c_str());
			}
		}
		else if(token.type == TT_STRING)
		{
			int index = props.Append(fbxProperty_t());
			fbxProperty_t &prop = props[index];
			prop.type = FBX_DATA_STRING;
			AllocArrayData(prop.data.a, token.Length() + 1);
			strncpy((char *)prop.data.a.ptr, token.c_str(), token.Length());
			((char *)prop.data.a.ptr)[token.Length()] = '\0';
			num++;
		}
		else // number
		{
			lexer->UnreadToken(&token);
			reqNum = true;
		}

		if(reqNum)
		{
			int64_t l;
			double d;
			int type = AsciiParseNumber(l, d);
			if(type <= 0)
			{
				lexer->Error("Parse number error\n");
			}
			else
			{
				int index = props.Append(fbxProperty_t());
				fbxProperty_t &prop = props[index];
				prop.type = type;
				num++;
				switch (type) {
					case FBX_DATA_BYTE:
						prop.data.b = (byte)l;
						break;
					case FBX_DATA_INT16:
						prop.data.h = (short)l;
						break;
					case FBX_DATA_BOOL:
						prop.data.z = l != 0;
						break;
					case FBX_DATA_CHAR:
						prop.data.c = (char)l;
						break;
					case FBX_DATA_INT32:
						prop.data.i = (int)l;
						break;
					case FBX_DATA_FLOAT32:
						prop.data.f = (float)d;
						break;
					case FBX_DATA_FLOAT64:
						prop.data.d = d;
						break;
					case FBX_DATA_INT64:
						prop.data.l = l;
						break;
					default:
						props.SetNum(index);
						num--;
						break;
				}
			}
		}
	}

	return num;
}

int idModelFbx::AsciiParseChildren(idList<fbxNode_t> &children)
{
	//common->Printf("AsciiParseChildren\n");
	AsciiSkipComment();

	if(!lexer->ExpectTokenString("{"))
		return -1;

	int num = 0;
	idToken token;

	while(true)
	{
		AsciiSkipComment();

		if(!ReadToken(&token))
		{
			lexer->Error("Read EOF when parsing ASCII fbx children\n");
			return -1;
		}
		if(token.type == TT_PUNCTUATION)
		{
			if(token == "}")
				break;

			lexer->Error("Expect punctuation '}' token, but found '%s'\n", token.c_str());
			return -1;
		}
		else if(token.type == TT_LITERAL || token.type == TT_NAME)
		{
			int index = children.Append(fbxNode_t());
			fbxNode_t &node = children[index];
			node.elem_id = token;
			if(AsciiParseNode(node))
				num++;
			else
				children.SetNum(index);
		}
		else
		{
			lexer->Error("Expect punctuation or name token, but found '%s'\n", token.c_str());
			return -1;
		}
	}

	return num;
}

bool idModelFbx::AsciiParseNode(fbxNode_t &node)
{
	//common->Printf("AsciiParseNode: %s\n", node.elem_id.c_str());
	node.elem_subtree = NULL;

	idList<fbxProperty_t> props;
	if(AsciiParseProperty(props) < 0)
		return false;

	node.elem_props.Swap(props);

	AsciiSkipComment();

	idToken token;

	if(!ReadToken(&token))
		return true;

	if(token.type == TT_PUNCTUATION)
	{
		if(token == "{")
		{
			lexer->UnreadToken(&token);

			idList<fbxNode_t> children;
			int num = AsciiParseChildren(children);
			if(num < 0)
				return false;
			if(num > 0)
			{
				node.elem_subtree = new idList<fbxNode_t>;
				node.elem_subtree->Swap(children);
			}
		}
		else
			lexer->UnreadToken(&token);
	}
	else
	{
		lexer->UnreadToken(&token);
	}

	return true;
}

bool idModelFbx::Parse(const char *filePath)
{
    if (ParseBinary(filePath)) {
		return true;
    }
    if (ParseAscii(filePath)) {
        return true;
    }
	return false;
}

bool idModelFbx::ParseBinary(const char *filePath)
{
    Clear();

    file = fileSystem->OpenFileRead(filePath);
    if(!file)
    {
        common->Warning("Load binary fbx file fail: %s", filePath);
        return false;
    }

    idList<fbxNode_t> root;

    bool err = false;
    do {
        fbxHeader_t header;
        if(ReadHeader(header) <= 0)
        {
            common->Warning("Read binary fbx header fail: %s", filePath);
            err = true;
            break;
        }

        fbx_version = header.fbx_version;
        //common->Printf("Binary fbx version: %u\n", fbx_version);

        fbxNode_t object;
        while(true)
        {
            if(read_elem(object))
                root.Append(object);
            else
                break;
        }
    } while(false);

    if(err)
    {
        Clear();
    }
    else
    {
        fileSystem->CloseFile(file);
        file = NULL;
		//Print(root);

        model.Convert(root);
    }

    for (int i = 0; i < root.Num(); ++i) {
        FreeObject(root[i]);
    }

    return !err;
}

bool idModelFbx::ParseAscii(const char *filePath)
{
    idLexer parser(/*LEXFL_NOFATALERRORS |*/ LEXFL_NOSTRINGESCAPECHARS);
    if(!parser.LoadFile(filePath))
    {
        common->Warning("Load ASCII fbx file fail: %s", filePath);
        return false;
    }

	lexer = &parser;

	bool err = false;
    idList<fbxNode_t> root;
	idToken token;

	while(true)
	{
		AsciiSkipComment();

		if(!ReadToken(&token))
			break;

		if(token.type == TT_PUNCTUATION)
		{
			if(token == "}")
				break;

			lexer->Error("Expect punctuation '}' token, but found '%s'\n", token.c_str());
			err = true;
			break;
		}
		else if(token.type == TT_LITERAL || token.type == TT_NAME)
		{
			int index = root.Append(fbxNode_t());
			fbxNode_t &node = root[index];
			node.elem_id = token;
			if(!AsciiParseNode(node))
				root.SetNum(index);
		}
		else
		{
			lexer->Error("Expect punctuation or name token, but found '%s' %d\n", token.c_str(), token.type);
			err = true;
			break;
		}
	}

    if(err)
    {
        Clear();
    }
    else
    {
        const fbxNode_t *header = FindObject(root, "FBXHeaderExtension");
        if(!header || !header->elem_subtree)
        {
            common->Warning("Read ASCII fbx header fail: %s", filePath);
            Clear();
            err = true;
        }
        else
        {
            const fbxNode_t *version = FindObject(*header->elem_subtree, "FBXVersion");
            if(!version || !version->elem_props.Num())
            {
                common->Warning("Read ASCII fbx version fail: %s", filePath);
                Clear();
                err = true;
            }
            else
            {
                fbx_version = GetProperty<unsigned int>(*version, 0, 0);
                //common->Printf("ASCII fbx version: %u\n", fbx_version);
                lexer = NULL;
                //Print(root);
                model.Convert(root);
            }
        }
    }

    for (int i = 0; i < root.Num(); ++i) {
        FreeObject(root[i]);
    }

    return !err;
}


const fbxNode_t * idModelFbx::FindObject(const idList<fbxNode_t> &list, const char *name)
{
	for(int i = 0; i < list.Num(); i++)
	{
		const fbxNode_t &obj = list[i];
		if(!idStr::Icmp(obj.elem_id, name))
			return &obj;
	}
	return NULL;
}

const fbxNode_t * idModelFbx::FindChild(const fbxNode_t &object, const char *name)
{
	if(!object.elem_subtree)
		return NULL;
	return FindObject(*object.elem_subtree, name);
}

int idModelFbx::GroupTriangle(idList<idList<idDrawVert> > &verts, idList<idList<int> > &faces, idStrList &mats, bool keepDup) const
{
	int num = 0;
	idList<int> matList;
	idList<vertTriGroup_s> objList;
    vertTriGroup_s *cur;

	const fbxObject &objects = model.Objects;
	const fbxGeometry &geometry = objects.Geometry;

	int LayerElementNormalId = -1;
	int LayerElementColorId = -1;
	int LayerElementUVId = -1;
	int LayerElementMaterialId = -1;
	int *layerIds[] = {
			&LayerElementNormalId,
			&LayerElementColorId,
			&LayerElementUVId,
			&LayerElementMaterialId,
	};
	const char *layerNames[] = {
			"LayerElementNormal",
			"LayerElementColor",
			"LayerElementUV",
			"LayerElementMaterial",
	};
	for(int n = 0; n < geometry.Layer.elements.Num(); n++)
	{
		const fbxLayerElement &layer = geometry.Layer.elements[n];
		for(int o = 0; o < sizeof(layerIds) / sizeof(layerIds[0]); o++)
		{
			if(!idStr::Icmp(layerNames[o], layer.Type))
				*layerIds[o] = layer.TypedIndex;
		}
	}
	const fbxLayerElement_TEMPLATE_TYPE(UV) *UV = NULL;
	const fbxLayerElement_TEMPLATE_TYPE(Normal) *Normal = NULL;
	const fbxLayerElement_TEMPLATE_TYPE(Material) *Material = NULL;
	if(LayerElementUVId != -1)
	{
		for(int n = 0; n < geometry.LayerElementUV.Num(); n++) {
			const fbxLayerElement_TEMPLATE_TYPE(UV) &uv = geometry.LayerElementUV[n];
			if (uv.id == LayerElementUVId) {
				UV = &uv;
				break;
			}
		}
	}
	if(LayerElementNormalId != -1)
	{
		for(int n = 0; n < geometry.LayerElementNormal.Num(); n++) {
			const fbxLayerElement_TEMPLATE_TYPE(Normal) &normal = geometry.LayerElementNormal[n];
			if (normal.id == LayerElementNormalId) {
				Normal = &normal;
				break;
			}
		}
	}
	if(LayerElementMaterialId != -1)
	{
		for(int n = 0; n < geometry.LayerElementMaterial.Num(); n++) {
			const fbxLayerElement_TEMPLATE_TYPE(Material) &material = geometry.LayerElementMaterial[n];
			if (material.id == LayerElementMaterialId) {
				Material = &material;
				break;
			}
		}
	}

	idList<int> vertexIndexes;
	const idList<int> *indexes;
	if(geometry.PolygonVertexIndex.Num() > 0)
		indexes = &geometry.PolygonVertexIndex;
	else
	{
		vertexIndexes.SetNum(geometry.Vertices.Num() / 3);
		for(int i = 0; i < vertexIndexes.Num(); i++)
			vertexIndexes[i] = i;
		if(vertexIndexes.Num() / 3 != 0)
			vertexIndexes.SetNum(vertexIndexes.Num() / 3 * 3); // must 3 multiply
		indexes = &vertexIndexes;
	}

	for(int n = 0; n < indexes->Num(); n++)
	{
		int idx = indexes->operator[](n);
		if(idx < 0) idx = -(idx+1);
		int oriIndex = idx;
		idx *= 3;

		// Material
		int matIndex = -1;
		if(Material)
		{
			if(Material->ByVertex())
				matIndex = Material->Materials[oriIndex];
			else
				matIndex = Material->Materials[n / 3];
		}

		int index = matList.FindIndex(matIndex);
		if(index < 0)
		{
			index = matList.Append(matIndex);
			objList.Append(vertTriGroup_s());
			verts.Append(idList<idDrawVert>());
			faces.Append(idList<int>());
			mats.Append(idStr());
			cur = &objList[index];
			cur->vert = &verts[index];
			cur->face = &faces[index];
			cur->mat = &mats[index];
			num++;
			if(matIndex >= 0 && matIndex < objects.Material.Num())
				*cur->mat = objects.Material[matIndex].name;
		}
		else
		{
			cur = &objList[index];
			if(!keepDup)
			{
				index = cur->origIndex.FindIndex(oriIndex);
				if(index != -1) // vertex exists
				{
					cur->face->Append(cur->newIndex[index]);
					continue;
				}
			}
		}

		index = cur->vert->Append(idDrawVert());
		idDrawVert &dv = cur->vert->operator[](index);
		if(!keepDup)
		{
			cur->origIndex.Append(oriIndex); // store original index
			cur->newIndex.Append(index); // store new index
		}

		// Vertex
		dv.xyz.Set(geometry.Vertices[idx], geometry.Vertices[idx+1], geometry.Vertices[idx+2]);
		// Index
		cur->face->Append(index);

		// UV
		dv.st.Set(0.0f, 0.0f);
		if(UV)
		{
			if(UV->Get(n, oriIndex, 2, geometry.Vertices.Num(), indexes->Num(), dv.st.ToFloatPtr()))
				dv.st.y = 1.0f - dv.st.y;
		}

		// Normal
		dv.normal.Set(0.0f, 0.0f, 0.0f);
		if(Normal)
		{
			Normal->Get(n, oriIndex, 3, geometry.Vertices.Num(), indexes->Num(), dv.normal.ToFloatPtr());
		}
	}

    return num;
}

bool idModelFbx::ToMd5Mesh(idMd5MeshFile &md5mesh, int flags, float scale, const idVec3 *meshOffset, const idMat3 *meshRotation) const
{
    int i, j;
    md5meshJoint_t *md5Bone;
    const fbxLimbNode *refBone;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
    const fbxObject &objects = model.Objects;
    const fbxGeometry &geometry = objects.Geometry;
    int numBones = objects.LimbNode.Num();
    const fbxCluster *cluster;
    const bool renameOrigin = flags & MD5CF_RENAME_ORIGIN;
    const bool addOrigin = flags & MD5CF_ADD_ORIGIN;
    assert(renameOrigin != addOrigin);

    bool usingBindPose = objects.BindPose.PoseNode.Num() > 0;
    bool usingClusterTransform = false;
	if(!usingBindPose)
	{
		usingClusterTransform = true;
		for (i = 0, refBone = &objects.LimbNode[0]; i < objects.LimbNode.Num(); i++, refBone++)
		{
            cluster = refBone->Cluster();
			if(cluster && cluster->TransformLink.Num() != 16)
			{
				usingClusterTransform = false;
				break;
			}
		}
	}

	idList<int64_t> poseNodeKeys;
	idList<const fbxPoseNode *> poseNodeValues;
	poseNodeKeys.SetNum(objects.BindPose.PoseNode.Num());
	poseNodeValues.SetNum(objects.BindPose.PoseNode.Num());
	for(j = 0; j < objects.BindPose.PoseNode.Num(); j++)
	{
		const fbxPoseNode *pose = &objects.BindPose.PoseNode[j];
		poseNodeKeys[j] = pose->Node;
		poseNodeValues[j] = pose;
	}

    md5mesh.Commandline() = va("Convert from fbx file: ");
    idStrList comments;
    if(addOrigin)
        comments.Append("addOrigin");
    if(renameOrigin)
        comments.Append("renameOrigin");
    if(scale > 0.0f)
        comments.Append(va("scale=%g", scale));
    if(meshOffset)
        comments.Append(va("offset=(%g %g %g)", meshOffset->x, meshOffset->y, meshOffset->z));
    if(meshRotation)
    {
        idAngles angle = meshRotation->ToAngles();
        comments.Append(va("rotation=(%g %g %g)", angle[0], angle[1], angle[2]));
    }
    idStr::Joint(md5mesh.Commandline(), comments, ", ");

    if(addOrigin)
        numBones++;

    // convert md5 joints
    idList<md5meshJoint_t> &md5Bones = md5mesh.Joints();
    md5Bones.SetNum(numBones);
    md5Bone = &md5Bones[0];
    if(addOrigin)
    {
        md5Bone->boneName = "origin";
        md5Bone->parentIndex = -1;
        md5Bone->pos.Zero();
        md5Bone->orient.Set(0.0f, 0.0f, 0.0f, 1.0f);
        md5Bone++;
    }

    bool hasOrigin = false;
    idList<const fbxLimbNode *> fbxBones;
    idList<idMat4> parentMats;

	idMat4 rootMat(
			fromdegrees(idVec3(objects.Mesh.Lcl_Rotation[0], objects.Mesh.Lcl_Rotation[1], objects.Mesh.Lcl_Rotation[2])).ToMat3(),
			idVec3(objects.Mesh.Lcl_Translation[0], objects.Mesh.Lcl_Translation[1], objects.Mesh.Lcl_Translation[2])
			);
	idMat4 scaleMat = mat4_identity;
	esScale((ESMatrix *)&scaleMat, objects.Mesh.Lcl_Scaling[0], objects.Mesh.Lcl_Scaling[1], objects.Mesh.Lcl_Scaling[2]);
	rootMat = rootMat * scaleMat;

    for (i = 0, refBone = &objects.LimbNode[0]; i < objects.LimbNode.Num(); i++, md5Bone++, refBone++)
    {
        md5Bone->boneName = refBone->name;
        fbxBones.Append(refBone);
        const fbxLimbNode *parentLimb = refBone->Parent();

        if(!parentLimb && !addOrigin)
        {
            if(hasOrigin)
            {
                common->Warning("Has more root bone: %d", i);
                md5Bone->parentIndex = 0;
            }
            else
            {
                md5Bone->parentIndex = -1;
                hasOrigin = true;
                if(renameOrigin)
                    md5Bone->boneName = "origin";
            }
        }
        else
        {
            md5Bone->parentIndex = fbxBones.FindIndex(parentLimb);
        }

        if(addOrigin)
            md5Bone->parentIndex += 1;

        boneOrigin.Set(0.0f, 0.0f, 0.0f);
        boneQuat.Set(0.0f, 0.0f, 0.0f, 1.0f);

        if(usingBindPose)
        {
			int index = poseNodeKeys.FindIndex(refBone->id);
			if(index != -1)
			{
				const fbxPoseNode *pose = poseNodeValues[index];
				idMat4 parent_matrix;
				if(!parentLimb)
				{
					parent_matrix = rootMat;
				}
				else
				{
					index = fbxBones.FindIndex(parentLimb);
					parent_matrix = parentMats[index];
				}

				idMat4 bind_matrix = pose->Matrix * parent_matrix.Inverse();
				parentMats.Append(pose->Matrix);

				SplitMatrixTransform(bind_matrix, boneOrigin, boneQuat);
			}
        }
		else if(usingClusterTransform)
        {
            cluster = refBone->Cluster();
            if(cluster && cluster->TransformLink.Num() == 16)
            {
                idMat4 parent_matrix;
                if(!parentLimb)
                {
                    parent_matrix = rootMat;
                }
                else
                {
                    int index = fbxBones.FindIndex(parentLimb);
                    parent_matrix = parentMats[index];
                }

                const idMat4 &matrix = *(const idMat4 *)(cluster->TransformLink.Ptr());
				idMat4 bind_matrix = matrix * parent_matrix.Inverse();
                parentMats.Append(matrix);

                SplitMatrixTransform(bind_matrix, boneOrigin, boneQuat);
            }
        }
        else
        {
            boneOrigin[0] = refBone->Lcl_Translation[0];
            boneOrigin[1] = refBone->Lcl_Translation[1];
            boneOrigin[2] = refBone->Lcl_Translation[2];

            boneQuat = fromdegrees(idVec3(refBone->Lcl_Rotation[0], refBone->Lcl_Rotation[1], refBone->Lcl_Rotation[2]));
        }

        if (!parentLimb)
        {
            if(meshRotation && !meshRotation->IsIdentity())
            {
                boneOrigin *= *meshRotation;
                boneQuat = (meshRotation->Transpose() * boneQuat.ToMat3()).ToQuat();
            }
            if(meshOffset && !meshOffset->IsZero())
                boneOrigin += *meshOffset;
        }
        if(scale > 0.0f)
            boneOrigin *= scale;

        md5Bone->pos = boneOrigin;

#if ETW_PSK
        md5Bone->orient = boneQuat;
#else
        md5Bone->orient = boneQuat.Inverse();
#endif

        md5Bone->orient.Normalize();

        if (md5Bone->parentIndex >= 0)
        {
            idVec3 rotated;
            idQuat quat;

            md5meshJoint_t *parent;

            parent = &md5Bones[md5Bone->parentIndex];

            idMat3 m = parent->orient.ToMat3();
#if ETW_PSK
            rotated = m.TransposeSelf() * md5Bone->pos;

            quat = parent->orient * md5Bone->orient;
#else
            rotated = m * md5Bone->pos;

            quat = md5Bone->orient * parent->orient;
#endif
            md5Bone->orient = quat.Normalize();
            md5Bone->pos = parent->pos + rotated;
        }
    }

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    // convert md5 mesh
    idList<md5meshMesh_t> &md5Meshes = md5mesh.Meshes();
    md5Meshes.SetNum(0);

    idList<int> matList;
    idList<vertGroup_s> objList;
    vertGroup_s *cur;

    int LayerElementUVId = -1;
    int LayerElementMaterialId = -1;
    int *layerIds[] = {
            &LayerElementUVId,
            &LayerElementMaterialId,
    };
    const char *layerNames[] = {
            "LayerElementUV",
            "LayerElementMaterial",
    };
    for(int n = 0; n < geometry.Layer.elements.Num(); n++)
    {
        const fbxLayerElement &layer = geometry.Layer.elements[n];
        for(int o = 0; o < sizeof(layerIds) / sizeof(layerIds[0]); o++)
        {
            if(!idStr::Icmp(layerNames[o], layer.Type))
                *layerIds[o] = layer.TypedIndex;
        }
    }
    const fbxLayerElement_TEMPLATE_TYPE(UV) *UV = NULL;
    const fbxLayerElement_TEMPLATE_TYPE(Normal) *Normal = NULL;
    const fbxLayerElement_TEMPLATE_TYPE(Material) *Material = NULL;
    if(LayerElementUVId != -1)
    {
        for(int n = 0; n < geometry.LayerElementUV.Num(); n++) {
            const fbxLayerElement_TEMPLATE_TYPE(UV) &uv = geometry.LayerElementUV[n];
            if (uv.id == LayerElementUVId) {
                UV = &uv;
                break;
            }
        }
    }
    if(LayerElementMaterialId != -1)
    {
        for(int n = 0; n < geometry.LayerElementMaterial.Num(); n++) {
            const fbxLayerElement_TEMPLATE_TYPE(Material) &material = geometry.LayerElementMaterial[n];
            if (material.id == LayerElementMaterialId) {
                Material = &material;
                break;
            }
        }
    }

    idList<int> vertexIndexes;
    const idList<int> *indexes;
    if(geometry.PolygonVertexIndex.Num() > 0)
        indexes = &geometry.PolygonVertexIndex;
    else
    {
        vertexIndexes.SetNum(geometry.Vertices.Num() / 3);
        for(i = 0; i < vertexIndexes.Num(); i++)
            vertexIndexes[i] = i;
        if(vertexIndexes.Num() / 3 != 0)
            vertexIndexes.SetNum(vertexIndexes.Num() / 3 * 3); // must 3 multiply
        indexes = &vertexIndexes;
    }

    idList<idList<vertWeight_s> > vertWeightGroups;
    vertWeightGroups.SetNum(geometry.Vertices.Num());

    for(j = 0, cluster = &objects.Cluster[0]; j < objects.Cluster.Num(); j++, cluster++)
    {
        for(i = 0; i < cluster->Indexes.Num(); i++)
        {
            int idx = cluster->Indexes[i];
            refBone = cluster->LimbNode();
            if(!refBone)
            {
                common->Warning("Cluster %s no limb\n", cluster->name.c_str());
                continue;
            }
            float weight = cluster->Weights[i];
            vertWeight_s w;
            w.weight = weight;
            w.limb = refBone;
            w.index = fbxBones.FindIndex(refBone);
            vertWeightGroups[idx].Append(w);
        }
    }

    md5meshMesh_t *mesh;
    for(j = 0; j < indexes->Num(); j+=3)
    {
        int md5VertIndexes[3];
        for(int k = 0; k < 3; k++)
        {
            int n = j + k;
            int idx = indexes->operator[](n);
            if(idx < 0) idx = -(idx+1);
            int oriIndex = idx; // vertexIndex
            const int vertexIndex = oriIndex;
            idx*=3;

            // Material
            int matIndex = -1;
            if(Material)
            {
                if(Material->ByVertex())
                    matIndex = Material->Materials[oriIndex];
                else
                    matIndex = Material->Materials[n / 3];
            }

            int index = matList.FindIndex(matIndex);
            if(index < 0)
            {
                index = matList.Append(matIndex);
                objList.Append(vertGroup_s());
                md5Meshes.Append(md5meshMesh_t());
                cur = &objList[index];
                cur->vert = &md5Meshes[index];
                mesh = cur->vert;
                if(matIndex >= 0 && matIndex < objects.Material.Num())
                    mesh->shader = objects.Material[matIndex].name;
            }
            else
            {
                cur = &objList[index];
                index = cur->origIndex.FindIndex(oriIndex);
                if(index != -1) // vertex exists
                {
                    md5VertIndexes[k] = cur->newIndex[index];
                    continue;
                }
            }

            // UV
            idVec2 uv(0.0f, 0.0f);
            if(UV)
            {
                if(UV->Get(n, oriIndex, 2, geometry.Vertices.Num(), indexes->Num(), uv.ToFloatPtr()))
                    uv.y = 1.0f - uv.y;
            }

            // Vertex
            const idList<vertWeight_s> &vertWeights = vertWeightGroups[oriIndex];
            md5meshVert_t md5Vert;
            md5Vert.uv = uv;
            md5Vert.weightIndex = mesh->weights.Num();
            md5Vert.weightElem = vertWeights.Num();
            index = mesh->verts.Append(md5Vert); // Add vert
            md5VertIndexes[k] = index;

            cur->origIndex.Append(oriIndex); // store original index
            cur->newIndex.Append(index); // store new index

            // Vertex
            idVec3 pos(geometry.Vertices[idx], geometry.Vertices[idx+1], geometry.Vertices[idx+2]);
            if(meshRotation && !meshRotation->IsIdentity())
                pos *= *meshRotation;
            if(meshOffset && !meshOffset->IsZero())
                pos += *meshOffset;
            if(scale > 0.0f)
                pos *= scale;

            // Weight
            float w = 0.0f;
            for(int m = 0; m < vertWeights.Num(); m++)
            {
                const vertWeight_s &weight = vertWeights[m];
                md5meshWeight_t md5Weight;
                md5Weight.jointIndex = weight.index;
                if(addOrigin)
                    md5Weight.jointIndex += 1;
                jointTransform = &jointTransforms[md5Weight.jointIndex];
                if(vertWeights.Num() == 1)
                {
                    if(weight.weight != 1.0f)
                        common->Warning("Vertex '%d' only 1 bone '%s' but weight is not 1 '%f'", vertexIndex, weight.limb->name.c_str(), weight.weight);
                    md5Weight.weightValue = 1.0f;
                }
                else
                    md5Weight.weightValue = weight.weight;

                w += md5Weight.weightValue;
                jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

                mesh->weights.Append(md5Weight); // Add weight
            }
            if(WEIGHTS_SUM_NOT_EQUALS_ONE(w))
            {
                common->Warning("Vertex '%d' weight sum is less than 1.0: %f", vertexIndex, w);
            }
        }
        md5meshTri_t md5Tri;
        md5Tri.vertIndex1 = md5VertIndexes[1];
        md5Tri.vertIndex2 = md5VertIndexes[0]; // swap
        md5Tri.vertIndex3 = md5VertIndexes[2];
        mesh->tris.Append(md5Tri); // Add tri
    }

    return true;
}

bool idModelFbx::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, int flags, float scale, const idVec3 *animOffset, const idMat3 *animRotation) const
{
	if(animIndex >= GetAnimCount())
		return false;

    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;
    const bool renameOrigin = flags & MD5CF_RENAME_ORIGIN;
    const bool addOrigin = flags & MD5CF_ADD_ORIGIN;
    assert(renameOrigin != addOrigin);
	const fbxAnimationStack &animStack = model.Objects.AnimationStack[animIndex];

	const int numFrames = animStack.NumFrames();

    const idList<md5meshJoint_t> _joints = md5mesh.Joints();
    const int numBones = _joints.Num();

    md5anim.FrameRate() = (int)24;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from fbx file: ");
    idStrList comments;
    if(addOrigin)
        comments.Append("addOrigin");
    if(renameOrigin)
        comments.Append("renameOrigin");
    if(scale > 0.0f)
        comments.Append(va("scale=%g", scale));
    if(animOffset)
        comments.Append(va("offset=(%g %g %g)", animOffset->x, animOffset->y, animOffset->z));
    if(animRotation)
    {
        idAngles angle = animRotation->ToAngles();
        comments.Append(va("rotation=(%g %g %g)", angle[0], angle[1], angle[2]));
    }
    idStr::Joint(md5anim.Commandline(), comments, ", ");

    // convert md5 joints
    idList<md5animHierarchy_t> &md5Bones = md5anim.Hierarchies();
    md5Bones.SetNum(numBones);

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    idStrList boneList;
    boneList.SetNum(numBones);
    for(i = 0, meshJoint = &_joints[0], md5Hierarchy = &md5Bones[0]; i < numBones; i++, meshJoint++, md5Hierarchy++)
    {
        boneList[i] = meshJoint->boneName;

        md5Hierarchy->boneName = meshJoint->boneName;
        md5Hierarchy->numComp = MD5ANIM_ALL;
        md5Hierarchy->frameIndex = i * 6;
        md5Hierarchy->parentIndex = meshJoint->parentIndex;
    }

    // convert md5 bounds
    idList<md5animBounds_t> &md5Bounds = md5anim.Bounds();
    md5Bounds.SetNum(numFrames);
    for(i = 0; i < md5Bounds.Num(); i++)
    {
        md5Bounds[i].Clear();
    }

    // convert md5 baseframe
    idList<md5animBaseframe_t> &md5Baseframes = md5anim.Baseframe();
    md5Baseframes.SetNum(numBones);
    for(i = 0, jointTransform = &jointTransforms[0], md5BaseFrame = &md5Baseframes[0]; i < md5Baseframes.Num(); i++, jointTransform++, md5BaseFrame++)
    {
        md5BaseFrame->xPos = jointTransform->t.x;
        md5BaseFrame->yPos = jointTransform->t.y;
        md5BaseFrame->zPos = jointTransform->t.z;
        idCQuat q = jointTransform->q.ToQuat().ToCQuat();
        md5BaseFrame->xOrient = q.x;
        md5BaseFrame->yOrient = q.y;
        md5BaseFrame->zOrient = q.z;
    }

    // convert md5 frames
    idList<md5animFrames_t> &md5Frames = md5anim.Frames();
    md5Frames.SetNum(numFrames);

	idList<const fbxAnimationCurveNode *> ll;
	animStack.FindConnections(ll, ANIMLAYER);
	const fbxAnimationLayer *animLayer = animStack.AnimationLayer();
	idList<const fbxAnimationCurveNode *> curveNodes;
	animLayer->AnimationCurveNode(curveNodes);
	idHashTable<xform_s> xformMap;
	for(i = 0; i < curveNodes.Num(); i++)
	{
		const fbxAnimationCurveNode *curveNode = curveNodes[i];
		const fbxLimbNode *limb = curveNode->LimbNode();
		if(!limb)
			continue;

        if(curveNode->type != 'T' && curveNode->type != 'R')
			continue;

		xform_s *rf = NULL;
		if(!xformMap.Get(limb->name, &rf))
		{
			xform_s f;
			f.T.node = f.R.node = NULL;
			f.T.x.curve = f.T.y.curve = f.T.z.curve = NULL;
			f.R.x.curve = f.R.y.curve = f.R.z.curve = NULL;
			xformMap.Set(limb->name, f);
			xformMap.Get(limb->name, &rf);
		}

		xformTrans_s *trans = curveNode->type == 'R' ? &rf->R : &rf->T;
		trans->node = curveNode;

		idList<const fbxAnimationCurve *> curves;
		curveNode->AnimationCurves(curves);
//		assert(curves.Num() == 3);
		if(curves.Num() > 0)
		{
			trans->x.curve = curves[0];
			trans->x.curve->GenFrameData(trans->x.keys, numFrames);
		}
		if(curves.Num() > 1)
		{
			trans->y.curve = curves[1];
			trans->y.curve->GenFrameData(trans->y.keys, numFrames);
		}
		if(curves.Num() > 2)
		{
			trans->z.curve = curves[2];
			trans->z.curve->GenFrameData(trans->z.keys, numFrames);
		}
	}

    for(i = 0; i < numFrames; i++)
    {
        md5animFrames_t &frames = md5Frames[i];
        frames.index = i;
        frames.joints.SetNum(numBones);

        idList<md5meshJoint_t> md5Joints = _joints;

        for(j = 0; j < numBones; j++)
        {
            if(j == 0 && addOrigin)
                continue;

            meshJoint = &_joints[j];

            idVec3 boneOrigin;
            idQuat boneQuat;
            md5Bone = &md5Joints[j];
            int index = j;
            if(addOrigin)
                index--;

            md5Bone->boneName = meshJoint->boneName;
            md5Bone->parentIndex = meshJoint->parentIndex;
            idStr meshBoneName = meshJoint->boneName;
            if(renameOrigin && j == 0)
            {
                meshBoneName = model.Objects.LimbNode[0].name;
            }

			xform_s *rf = NULL;
			if(xformMap.Get(meshBoneName, &rf))
			{
				if(rf->T.node)
				{
					boneOrigin[0] = rf->T.x.curve ? rf->T.x.keys[i] : rf->T.node->x;
					boneOrigin[1] = rf->T.y.curve ? rf->T.y.keys[i] : rf->T.node->y;
					boneOrigin[2] = rf->T.z.curve ? rf->T.z.keys[i] : rf->T.node->z;
				}
				else
					boneOrigin.Zero();
				if(rf->R.node)
				{
					idVec3 rot;
					rot[0] = rf->R.x.curve ? rf->R.x.keys[i] : rf->R.node->x;
					rot[1] = rf->R.y.curve ? rf->R.y.keys[i] : rf->R.node->y;
					rot[2] = rf->R.z.curve ? rf->R.z.keys[i] : rf->R.node->z;
					boneQuat = fromdegrees(rot);
				}
				else
					boneQuat.Set(0.0f, 0.0f, 0.0f, 1.0f);
			}
			else
			{
				if(i == 0)
					common->Warning("Bone not found in fbx animation %d: %s", animIndex, meshJoint->boneName.c_str());
				boneOrigin.Zero();
				boneQuat.Set(0.0f, 0.0f, 0.0f, 1.0f);
			}

			int rootIndex = addOrigin ? 0 : -1;
			if (md5Bone->parentIndex == rootIndex)
			{
				if(animRotation && !animRotation->IsIdentity())
				{
					boneOrigin *= *animRotation;
					boneQuat = (animRotation->Transpose() * boneQuat.ToMat3()).ToQuat();
				}
				if(animOffset && !animOffset->IsZero())
					boneOrigin += *animOffset;
			}
            if(scale > 0.0f)
                boneOrigin *= scale;

            md5Bone->pos = boneOrigin;

#if ETW_PSK
            md5Bone->orient = boneQuat;
#else
            md5Bone->orient = boneQuat.Inverse();
#endif

            md5Bone->orient.Normalize();

            if (md5Bone->parentIndex >= 0)
            {
                idVec3 rotated;
                idQuat quat;

                md5meshJoint_t *parent = &md5Joints[md5Bone->parentIndex];

                idMat3 m = parent->orient.ToMat3();
#if ETW_PSK
                rotated = m.TransposeSelf() * md5Bone->pos;

                quat = parent->orient * md5Bone->orient;
#else
                rotated = m * md5Bone->pos;

                quat = md5Bone->orient * parent->orient;
#endif
                md5Bone->orient = quat.Normalize();
                md5Bone->pos = parent->pos + rotated;
            }
        }

        idList<md5meshJointTransform_t> frameTransforms;
        idMd5MeshFile::ConvertJointTransforms(md5Joints, frameTransforms);

        // calc frame bounds
        md5mesh.CalcBounds(frameTransforms, md5Bounds[i]);

        md5Frame = &frames.joints[0];
        for(int m = 0; m < numBones; m++, md5Frame++)
        {
            frameTransform = &frameTransforms[m];
            idVec3 t;
            idCQuat q;

            t = frameTransform->t;
            q = frameTransform->q.ToCQuat();

            md5Frame->xPos = t.x;
            md5Frame->yPos = t.y;
            md5Frame->zPos = t.z;
            md5Frame->xOrient = q.x;
            md5Frame->yOrient = q.y;
            md5Frame->zOrient = q.z;
        }
    }

    return true;
}

int idModelFbx::ToMd5AnimList(idList<idMd5AnimFile> &md5anims, idMd5MeshFile &md5mesh, int flags, float scale, const idVec3 *offset, const idMat3 *rotation) const
{
	int num = 0;

	for(int i = 0; i < GetAnimCount(); i++)
	{
		int index = md5anims.Append(idMd5AnimFile());
		if(!ToMd5Anim(md5anims[index], md5mesh, i, flags, scale, offset, rotation))
		{
			md5anims.RemoveIndex(index);
			common->Warning("Animation '%s' convert fail", GetAnim(index));
		}
		else
			num++;
	}
	return num;
}

bool idModelFbx::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, int flags, float scale, const idVec3 *offset, const idMat3 *rotation) const
{
	int i = GetAnimIndex(animName);
	if(i >= 0)
		return ToMd5Anim(md5anim, md5mesh, i, flags, scale, offset, rotation);
	else
	{
		common->Warning("Animation '%s' not found in fbx", animName);
		return false;
	}
}

#ifdef _MODEL_OBJ
bool idModelFbx::ToObj(objModel_t &objModel) const
{
	idList<idList<idDrawVert> > verts;
	idList<idList<int> > faces;
	idStrList mats;
	int num = GroupTriangle(verts, faces, mats, true);

    if(num <= 0)
        return false;

	objModel.objects.SetNum(num);
    for(int i = 0; i < num; i++)
    {
		objModel.objects[i] = new objObject_t;
		objObject_t *objObject = objModel.objects[i];
        const idList<idDrawVert> &vert = verts[i];
        const idList<int> &face = faces[i];
		objObject->material = mats[i];

		objObject->indexes.SetNum(face.Num());
		for(int m = 0; m < face.Num(); m++)
		{
			objObject->indexes[m] = face[m];
		}
		objObject->vertexes.SetNum(vert.Num());
		objObject->texcoords.SetNum(vert.Num());
		objObject->normals.SetNum(vert.Num());
		const idDrawVert *dv = &vert[0];
		for(int m = 0; m < vert.Num(); m++, dv++)
		{
			objObject->vertexes[m] = dv->xyz;
			objObject->texcoords[m] = dv->st;
			objObject->normals[m] = dv->normal;
		}
	}

    return true;
}
#endif

void idModelFbx::PrintObject(int index, const fbxNode_t &object, int intent)
{
    idStr str;
    if(intent)
    {
        str.Fill(' ', intent * 2);
    }
    Sys_Printf("%s%d: %s\n", str.c_str(), index, object.elem_id.c_str()/*, object.elem_subtree ? object.elem_subtree->Num() : 0*/);
    for (int i = 0; i < object.elem_props.Num(); i++) {
		idStr name;
		switch(object.elem_props[i].type)
		{
			case FBX_DATA_STRING:
				name.Append((const char *)object.elem_props[i].data.a.ptr/*, object.elem_props[i].data.a.length*/);
				break;
			case FBX_DATA_BINARY:
				name += "length=";
				name += object.elem_props[i].data.a.length;
			case FBX_DATA_BYTE:
				name += (int)object.elem_props[i].data.b;
				break;
			case FBX_DATA_INT16:
				name += (int)object.elem_props[i].data.h;
				break;
			case FBX_DATA_BOOL:
				name += object.elem_props[i].data.z;
				break;
			case FBX_DATA_CHAR:
				name += object.elem_props[i].data.c;
				break;
			case FBX_DATA_INT32:
				name += object.elem_props[i].data.i;
				break;
			case FBX_DATA_FLOAT32:
				name += object.elem_props[i].data.f;
				break;
			case FBX_DATA_FLOAT64:
				name += va("%lf", object.elem_props[i].data.d);
				break;
			case FBX_DATA_INT64:
				name += va("%lld", (long long)object.elem_props[i].data.l);
				break;
			case FBX_ARRAY_BOOL:
			case FBX_ARRAY_BYTE:
			case FBX_ARRAY_INT32:
			case FBX_ARRAY_INT64:
			case FBX_ARRAY_FLOAT32:
			case FBX_ARRAY_FLOAT64:
				name += "[";
				name += object.elem_props[i].data.a.length;
				name += "]";
				break;
		}
        Sys_Printf("%s|-%d: %c(%d): %s\n", str.c_str(), i, object.elem_props[i].type, object.elem_props[i].type, name.c_str());
    }
    if(object.elem_subtree)
    {
        Sys_Printf("%s|-children: %d\n", str.c_str(), object.elem_subtree->Num());
        //Sys_Printf("%s|-children: %d\n", str.c_str(), object.elem_subtree->Num());
        for (int i = 0; i < object.elem_subtree->Num(); i++) {
            PrintObject(i, object.elem_subtree->operator[](i), intent + 1);
        }
    }
    Sys_Printf("%s------------------------------------------------------\n", str.c_str());
}

void idModelFbx::Print(void) const
{
    Sys_Printf("Version: %u\n", fbx_version);
    model.Print();
}

void idModelFbx::Print(const idList<fbxNode_t> &root) const
{
    for (int i = 0; i < root.Num(); i++) {
        PrintObject(i, root[i]);
    }
}

bool idModelFbx::PropertyIsTypes(const fbxNode_t &object, const char *types)
{
	int len = strlen(types);
	if(len > object.elem_props.Num())
		return false;
	for(int i = 0; i < len; i++)
	{
		if(types[i] == ' ' || types[i] == '*' || types[i] == '?')
			continue;
		if(object.elem_props[i].type != types[i])
			return false;
	}
	return true;
}

bool idModelFbx::PropertyIsType(const fbxNode_t &object, int index, byte type)
{
	if(index < 0)
		index = object.elem_props.Num() + index;
	if(index < 0 || index >= object.elem_props.Num())
		return false;
	return object.elem_props[index].type == type;
}

const char * idModelFbx::GetAnim(unsigned int index) const
{
    return model.Objects.AnimName(index);
}

int idModelFbx::GetAnimIndex(const char *name) const
{
	for(int i = 0; i < GetAnimCount(); i++)
	{
		if(!idStr::Icmp(name, GetAnim(i)))
			return i;
	}
	return -1;
}

int idModelFbx::GetAnimCount(void) const
{
    return model.Objects.AnimCount();
}

#ifdef _MODEL_OBJ
static void R_ConvertFbxToObj_f(const idCmdArgs &args)
{
    const char *filePath = args.Argv(1);
    idModelFbx fbx;
    if(fbx.Parse(filePath))
    {
        //fbx.Print();
        objModel_t objModel;
        if(fbx.ToObj(objModel))
        {
            idStr objPath = filePath;
            objPath.SetFileExtension(".obj");
            OBJ_Write(&objModel, objPath.c_str());
            common->Printf("Convert obj successful: %s -> %s\n", filePath, objPath.c_str());
        }
        else
            common->Warning("Convert obj fail: %s", filePath);
    }
    else
        common->Warning("Parse fbx fail: %s", filePath);
}
#endif

static int R_ConvertFbxToMd5(const char *filePath, bool doMesh = true, const idStrList *animList = NULL, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    int ret = 0;

    idModelFbx fbx;
    idMd5MeshFile md5MeshFile;
    bool meshRes = false;
    if(fbx.Parse(filePath))
    {
        //fbx.Print();
        if(fbx.ToMd5Mesh(md5MeshFile, flags, scale, offset, rotation))
        {
            if(doMesh)
            {
                md5MeshFile.Commandline().Append(va(" - %s", filePath));
                idStr md5meshPath = R_Model_MakeOutputPath(filePath, "." MD5_MESH_EXT, savePath);
                md5MeshFile.Write(md5meshPath.c_str());
                common->Printf("Convert md5mesh successful: %s -> %s\n", filePath, md5meshPath.c_str());
                ret++;
            }
            else
            {
                common->Printf("Convert md5mesh successful: %s\n", filePath);
            }
            meshRes = true;
        }
        else
            common->Warning("Convert md5mesh fail: %s", filePath);
    }
    else
        common->Warning("Parse fbx fail: %s", filePath);

    if(!meshRes)
        return ret;

    if(!animList)
        return ret;

    idStrList list;
    if(!animList->Num()) // all
    {
        for(int i = 0; i < fbx.GetAnimCount(); i++)
            list.Append(va("%d", i));
    }
    else
        list = *animList;

    for(int i = 0; i < list.Num(); i++)
    {
        const char *anim = list[i];
        bool isNumber = true;
        for(int m = 0; m < strlen(anim); m++)
        {
            if(!isdigit(anim[m]))
            {
                isNumber = false;
                break;
            }
        }
        idMd5AnimFile md5AnimFile;
        bool ok;
        const char *animName;
        idStr md5animPath;
        if(isNumber)
        {
            int index = atoi(anim);
            animName = fbx.GetAnim(index);
            if(!animName)
            {
                common->Warning("Invalid animation index '%d'", index);
                continue;
            }
            common->Printf("Convert fbx animation to md5anim: %d -> %s\n", index, animName);
            ok = fbx.ToMd5Anim(md5AnimFile, md5MeshFile, index, flags, scale, offset, rotation);
            md5animPath = filePath;
            md5animPath.StripFilename();
            md5animPath.AppendPath(animName);
            md5animPath = R_Model_MakeOutputPath(md5animPath, "." MD5_ANIM_EXT, savePath);
        }
        else
        {
            idStr name = anim;
            name.StripPath();
            name.StripFileExtension();
            common->Printf("Convert fbx animation to md5anim: %s -> %s\n", anim, name.c_str());
            ok = fbx.ToMd5Anim(md5AnimFile, md5MeshFile, name.c_str(), flags, scale, offset, rotation);
            animName = anim;
            md5animPath = anim;
            md5animPath = R_Model_MakeOutputPath(anim, "." MD5_ANIM_EXT, savePath);
        }
        if(ok)
        {
            md5AnimFile.Commandline().Append(va(" - %s", animName));
            md5AnimFile.Write(md5animPath.c_str());
            common->Printf("Convert md5anim successful: %s -> %s\n", animName, md5animPath.c_str());
            ret++;
        }
        else
            common->Warning("Convert md5anim fail: %s", animName);
    }

    return ret;
}

ID_INLINE static int R_ConvertFbxMesh(const char *filePath, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertFbxToMd5(filePath, true, NULL, flags, scale, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertFbxAnim(const char *filePath, const idStrList &animList, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertFbxToMd5(filePath, false, &animList, flags, scale, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertFbx(const char *filePath, const idStrList &animList, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertFbxToMd5(filePath, true, &animList, flags, scale, offset, rotation, savePath);
}

static void R_ConvertFbxToMd5mesh_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(fbx), args.Argv(0));
        return;
    }

    idStr mesh;
    int flags = 0;
    float scale = -1.0f;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStrList anims;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &flags, &scale, &offset, &rotation, NULL, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(fbx), args.Argv(0));
        return;
    }
    R_ConvertFbxMesh(mesh, flags, scale, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertFbxToMd5anim_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5ANIM_ALL_USAGE(fbx), args.Argv(0));
        return;
    }

    idStr mesh;
    int flags = 0;
    float scale = -1.0f;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStrList anims;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &flags, &scale, &offset, &rotation, &anims, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5ANIM_ALL_USAGE(fbx), args.Argv(0));
        return;
    }
    R_ConvertFbxAnim(mesh, anims, flags, scale, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertFbxToMd5_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5_ALL_USAGE(fbx), args.Argv(0));
        return;
    }

    idStr mesh;
    int flags = 0;
    float scale = -1.0f;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStrList anims;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &flags, &scale, &offset, &rotation, &anims, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5_ALL_USAGE(fbx), args.Argv(0));
        return;
    }
    R_ConvertFbx(mesh, anims, flags, scale, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

bool R_Model_HandleFbx(const md5ConvertDef_t &convert)
{
    if(R_ConvertFbx(convert.mesh, convert.anims,
                convert.flags,
                convert.scale,
                convert.offset.IsZero() ? NULL : &convert.offset,
                convert.rotation.IsIdentity() ? NULL : &convert.rotation,
                convert.savePath.IsEmpty() ? NULL : convert.savePath.c_str()
    ) != 1 + convert.anims.Num())
    {
        common->Warning("Convert fbx to md5mesh/md5anim fail in entityDef '%s'", convert.def->GetName());
        return false;
    }
    return true;
}

static void ArgCompletion_fbx(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".fbx"
            , NULL);
}

void R_Fbx_AddCommand(void)
{
    cmdSystem->AddCommand("fbxToMd5mesh", R_ConvertFbxToMd5mesh_f, CMD_FL_RENDERER, "Convert fbx to md5mesh", ArgCompletion_fbx);
    cmdSystem->AddCommand("fbxToMd5anim", R_ConvertFbxToMd5anim_f, CMD_FL_RENDERER, "Convert fbx to md5anim", ArgCompletion_fbx);
    cmdSystem->AddCommand("fbxToMd5", R_ConvertFbxToMd5_f, CMD_FL_RENDERER, "Convert fbx to md5mesh/md5anim", ArgCompletion_fbx);
#ifdef _MODEL_OBJ
    cmdSystem->AddCommand("fbxToObj", R_ConvertFbxToObj_f, CMD_FL_RENDERER, "Convert fbx to obj", ArgCompletion_fbx);
#endif
}
