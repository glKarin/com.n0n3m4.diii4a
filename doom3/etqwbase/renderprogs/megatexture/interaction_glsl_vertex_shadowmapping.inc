$if !defined(SHADOW_MAPPING_INCLUDEAD)
$define SHADOW_MAPPING_INCLUDEAD

$define u_lightType $u_uniformIntParm7.x
$define globalLightOrigin $viewOriginWorld
$define var_ShadowCoord var_VertexPosition
$define shadowMVPMatrix $u_uniformMatrixParm5

$define LT_PARALLEL_LIGHT 1
$define LT_POINT_LIGHT 2
$define LT_SPOT_LIGHT 3
$define _POINT_LIGHT (u_lightType == LT_POINT_LIGHT)
$define _PARALLEL_LIGHT (u_lightType == LT_PARALLEL_LIGHT)
$define _SPOT_LIGHT (u_lightType == LT_SPOT_LIGHT)

$ifdef _GLES2
varying highp vec3 var_LightToVertex;
$else
varying highp vec3 var_VertexToLight;
$endif

varying highp vec4 var_ViewPosition;
varying highp vec4 var_VertexPosition;

$else

    if (_POINT_LIGHT)
    {
        highp vec4 posInLight = $u_modelMatrix * attr_Vertex;
$ifdef _GLES2
        var_LightToVertex = posInLight.xyz - globalLightOrigin.xyz;
$else
        var_VertexToLight = globalLightOrigin.xyz - posInLight.xyz;
$endif
        var_VertexPosition = attr_Vertex;
    }
    else if (_PARALLEL_LIGHT)
    {
$ifdef _GLES2
        var_ShadowCoord = attr_Vertex * shadowMVPMatrix;
$else
        var_ViewPosition = $u_modelViewMatrix * attr_Vertex;
        var_VertexPosition = attr_Vertex;
$endif
    }
    else if (_SPOT_LIGHT)
    {
        var_ShadowCoord = attr_Vertex * shadowMVPMatrix;
    }

$endif // SHADOW_MAPPING_INCLUDEAD
