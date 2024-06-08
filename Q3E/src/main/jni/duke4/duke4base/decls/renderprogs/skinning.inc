//--------------------------------------------------------------
// GPU transformation of the normal / binormal / bitangent
//
// multiplying with 255.1 give us the same result and is faster than floor( w * 255 + 0.5 )
//--------------------------------------------------------------
float w0 = attr_Color2.x;
float w1 = attr_Color2.y;
float w2 = attr_Color2.z;
float w3 = attr_Color2.w;

vec4 matX, matY, matZ;// must be vec4 for vec4
float joint = attr_Color.x * 255.1 * 3.0;
matX = matrices[int(joint+0.0)] * w0;
matY = matrices[int(joint+1.0)] * w0;
matZ = matrices[int(joint+2.0)] * w0;

joint = attr_Color.y * 255.1 * 3.0;
matX += matrices[int(joint+0.0)] * w1;
matY += matrices[int(joint+1.0)] * w1;
matZ += matrices[int(joint+2.0)] * w1;

joint = attr_Color.z * 255.1 * 3.0;
matX += matrices[int(joint+0.0)] * w2;
matY += matrices[int(joint+1.0)] * w2;
matZ += matrices[int(joint+2.0)] * w2;

joint = attr_Color.w * 255.1 * 3.0;
matX += matrices[int(joint+0.0)] * w3;
matY += matrices[int(joint+1.0)] * w3;
matZ += matrices[int(joint+2.0)] * w3;

vec3 vNormal;
vNormal.x = dot3( matX, vModelNormal );
vNormal.y = dot3( matY, vModelNormal );
vNormal.z = dot3( matZ, vModelNormal );
vNormal = normalize( vNormal );

vec3 vTangent;
vTangent.x = dot3( matX, vModelTangent );
vTangent.y = dot3( matY, vModelTangent );
vTangent.z = dot3( matZ, vModelTangent );
vTangent = normalize( vTangent );

vec3 vBinormal;
vBinormal.x = dot3( matX, vModelBinormal );
vBinormal.y = dot3( matY, vModelBinormal );
vBinormal.z = dot3( matZ, vModelBinormal );
vBinormal = normalize( vBinormal );

vec4 vertex_position;
vertex_position.x = dot4( matX, attr_Position );
vertex_position.y = dot4( matY, attr_Position );
vertex_position.z = dot4( matZ, attr_Position );
vertex_position.w = 1.0;
