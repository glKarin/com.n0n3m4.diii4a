/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2019 Justin Marshall (IcedTech)
Copyright (C) 2021 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop


#include "Model_obj.h"

/*
======================================================================

	Parses Wavefront export files. This parser is designed to work with Blender and 3D Studio Max


	The goal is to parse the information into memory exactly as it is
	represented in the file.  Users of the data will then move it
	into a form that is more convenient for them.

======================================================================
*/


/*
=================
OBJ_Parse
=================
*/
objModel_t* OBJ_Parse( const char* fileName, const char* objFileBuffer, int length )
{
	objModel_t* model = new objModel_t;

	idLexer src;
	idToken	token, token2;

	// total
	idList<idVec3> vertexes;
	idList<idVec2> texCoords;
	idList<idVec3> normals;

	int numCollectedVerts = 0;

	// current object or group
	//idList<idDrawVert>	drawVerts;
	idList<idVec3>		objVertexes;
	idList<idVec2>		objTexCoords;
	idList<idVec3>		objNormals;
	idList<glIndex_t>	objIndices;

	idStr				material;
	float scale = 1.0f;
	bool transpose_vertex = false;

	src.LoadMemory( objFileBuffer, length, fileName, 0 );

	while( true )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}

		if( token == "v" )
		{
			idVec3 vertex;

			if(transpose_vertex)
			{
				vertex.x = src.ParseFloat();
				vertex.z = src.ParseFloat();
				vertex.y = -src.ParseFloat();
			}
			else
			{
				vertex.x = src.ParseFloat();
				vertex.y = src.ParseFloat();
				vertex.z = src.ParseFloat();
			}
			if(scale != 1.0f)
				vertex *= scale;
			vertexes.Append( vertex );
		}
		else if( token == "vt" )
		{
			idVec2 st;
			st.x = src.ParseFloat();
			st.y = 1.0f - src.ParseFloat();
            src.SkipRestOfLine(); //k: for 3 components
			texCoords.Append( st );
		}
		else if( token == "#" )
		{
			idStr line;

			// karin: extra arguments in comment
			// # parm scale 100
			// # parm transpose_vertex
			if(src.ReadTokenOnLine(&token2))
			{
				if(!idStr::Icmp(token2, "parm"))
				{
					if(src.ReadTokenOnLine(&token2))
					{
						if(!idStr::Icmp(token2, "scale"))
						{
							bool err = false;
							float f = src.ParseFloat(&err);
							if(!err)
								scale = f;
						}
						else if(!idStr::Icmp(token2, "transpose_vertex"))
						{
							transpose_vertex = true;
						}
					}
				}
			}

			// Skip comments
			src.ReadRestOfLine( line );
		}
		else if( token == "mtllib" )
		{
			idStr line;

			// We don't use obj materials.
			src.ReadRestOfLine( line );
		}
		else if( token == "s" )
		{
			idStr line;
			src.ReadRestOfLine( line );
		}
		else if( token == "l" )
		{
			// We don't support lines
			idStr line;
			src.ReadRestOfLine( line );
		}
		else if( token == "o" ) // new object
		{
			idStr line;
			src.ReadRestOfLine( line );

			if( objVertexes.Num() )
			{
				objObject_t* obj = new objObject_t;

				obj->material = material;
				obj->vertexes = objVertexes;
				obj->texcoords = objTexCoords;
				obj->normals = objNormals;
				obj->indexes = objIndices;

				numCollectedVerts += objVertexes.Num();

				objVertexes.Clear();
				objTexCoords.Clear();
				objNormals.Clear();
				objIndices.Clear();

				model->objects.Append( obj );
			}
            // remove read vertexes/texcoords/normals
            vertexes.Clear();
            texCoords.Clear();
            normals.Clear();
		}
        else if( token == "g" ) // new face group
        {
            idStr line;
            src.ReadRestOfLine( line );

            if( objVertexes.Num() )
            {
                objObject_t* obj = new objObject_t;

                obj->material = material;
                obj->vertexes = objVertexes;
                obj->texcoords = objTexCoords;
                obj->normals = objNormals;
                obj->indexes = objIndices;

                numCollectedVerts += objVertexes.Num();

                objVertexes.Clear();
                objTexCoords.Clear();
                objNormals.Clear();
                objIndices.Clear();

                model->objects.Append( obj );
            }
        }
		else if( token == "usemtl" )
		{
			idStr line;
			src.ReadRestOfLine( line );

			material = line;
		}
		else if( token == "vn" )
		{
			idVec3 normal;
			normal.x = src.ParseFloat();
			normal.y = src.ParseFloat();
			normal.z = src.ParseFloat();
			normals.Append( normal );
		}
		else if( token == "f" )
		{
			idStr line;
			int vertexIndex[3];
			int uvIndex[3];
			int normalIndex[3];

			src.ReadRestOfLine( line );

			if( sscanf( line.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] ) == 9 )
            {
                for( int i = 0; i < 3; i++ )
                {
                    idDrawVert drawVert;

                    int vertidx = vertexIndex[i] - 1;
                    int texidx = uvIndex[i] - 1;
                    int normalidx = normalIndex[i] - 1;

                    //drawVert.xyz = vertexes[vertidx];
                    //drawVert.SetTexCoord( texCoords[texidx] );
                    //drawVert.SetNormal( normals[normalidx] );

                    objVertexes.Append( vertexes[vertidx] );
                    objTexCoords.Append( texCoords[texidx] );
                    objNormals.Append( normals[normalidx] );

                    //objIndices.Append( vertidx - numCollectedVerts );
                    objIndices.Append( objIndices.Num() );
                }
            }
            else if(sscanf( line.c_str(), "%d/%d %d/%d %d/%d", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2] ) == 6)
			{
                for( int i = 0; i < 3; i++ )
                {
                    idDrawVert drawVert;

                    int vertidx = vertexIndex[i] - 1;
                    int texidx = uvIndex[i] - 1;

                    //drawVert.xyz = vertexes[vertidx];
                    //drawVert.SetTexCoord( texCoords[texidx] );

                    objVertexes.Append( vertexes[vertidx] );
                    objTexCoords.Append( texCoords[texidx] );
                    objNormals.Append( idVec3(0.0f, 0.0f, 0.0f) );

                    //objIndices.Append( vertidx - numCollectedVerts );
                    objIndices.Append( objIndices.Num() );
                }
            }
            else if(sscanf( line.c_str(), "%d//%d %d//%d %d//%d", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2] ) == 6)
            {
                for( int i = 0; i < 3; i++ )
                {
                    idDrawVert drawVert;

                    int vertidx = vertexIndex[i] - 1;
                    int normalidx = normalIndex[i] - 1;

                    //drawVert.xyz = vertexes[vertidx];
                    //drawVert.SetNormal( normals[normalidx] );

                    objVertexes.Append( vertexes[vertidx] );
                    objTexCoords.Append( idVec2(0.0f, 0.0f) );
                    objNormals.Append( normals[normalidx] );

                    //objIndices.Append( vertidx - numCollectedVerts );
                    objIndices.Append( objIndices.Num() );
                }
            }
            else if(sscanf( line.c_str(), "%d %d %d", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2] ) == 3)
            {
                for( int i = 0; i < 3; i++ )
                {
                    idDrawVert drawVert;

                    int vertidx = vertexIndex[i] - 1;

                    //drawVert.xyz = vertexes[vertidx];

                    objVertexes.Append( vertexes[vertidx] );
                    objTexCoords.Append( idVec2(0.0f, 0.0f) );
                    objNormals.Append( idVec3(0.0f, 0.0f, 0.0f) );

                    //objIndices.Append( vertidx - numCollectedVerts );
                    objIndices.Append( objIndices.Num() );
                }
			}
            else
                common->FatalError( "Failed to parse face line in line %d", src.GetLineNum() );
		}
		else
		{
			common->FatalError( "idRenderModelStatic::ParseOBJ: Unknown or unexpected token %s in line %d", token.c_str(), src.GetLineNum() );
		}
	}

	if( objVertexes.Num() )
	{
		objObject_t* obj = new objObject_t;

		obj->material = material;

		obj->vertexes = objVertexes;
		obj->texcoords = objTexCoords;
		obj->normals = objNormals;
		obj->indexes = objIndices;

		model->objects.Append( obj );
	}

	return model;
}

/*
=================
OBJ_Load
=================
*/
objModel_t* OBJ_Load( const char* fileName )
{
	char* objBuffer;
	ID_TIME_T timeStamp;
	objModel_t* obj;

	int objBufferLen = fileSystem->ReadFile( fileName, ( void** )&objBuffer, &timeStamp );
	if( objBufferLen <= 0 || objBuffer == NULL )
	{
		return NULL;
	}

	obj = OBJ_Parse( fileName, objBuffer, objBufferLen );
	obj->timeStamp = timeStamp;

	fileSystem->FreeFile( objBuffer );

	return obj;
}

/*
=================
OBJ_Free
=================
*/
void OBJ_Free( objModel_t* model )
{
	if( !model )
	{
		return;
	}

	for( int i = 0; i < model->objects.Num(); i++ )
	{
		objObject_t* obj = model->objects[i];

		delete obj;
	}

	model->objects.Clear();

	delete model;
}

/*
=================
OBJ_Write
=================
*/
static void OBJ_WriteObject( idFile *file, const objObject_t* obj )
{
    file->Printf("usemtl %s", obj->material.c_str());
    file->Write("\n\n", 2);

    file->Printf("# vertex num %d", obj->vertexes.Num());
    file->Write("\n", 1);
    for(int i = 0; i < obj->vertexes.Num(); i++)
    {
        file->Printf("v %f %f %f", obj->vertexes[i].x, obj->vertexes[i].y, obj->vertexes[i].z);
        file->Write("\n", 1);
    }
    file->Write("\n", 1);

    file->Printf("# texcoord num %d", obj->texcoords.Num());
    file->Write("\n", 1);
    for(int i = 0; i < obj->texcoords.Num(); i++)
    {
        file->Printf("vt %f %f", obj->texcoords[i].x, 1.0f - obj->texcoords[i].y);
        file->Write("\n", 1);
    }
    file->Write("\n", 1);

    file->Printf("# normal num %d", obj->normals.Num());
    file->Write("\n", 1);
    for(int i = 0; i < obj->normals.Num(); i++)
    {
        file->Printf("vn %f %f %f", obj->normals[i].x, obj->normals[i].y, obj->normals[i].z);
        file->Write("\n", 1);
    }
    file->Write("\n", 1);

    file->Printf("# face num %d", obj->indexes.Num());
    file->Write("\n", 1);
    for(int i = 0; i < obj->indexes.Num(); i += 3)
    {
        int index1 = obj->indexes[i] + 1;
        int index2 = obj->indexes[i + 1] + 1;
        int index3 = obj->indexes[i + 2] + 1;
        file->Printf("f %d/%d/%d %d/%d/%d %d/%d/%d"
                     , index1, index1, index1
                     , index2, index2, index2
                     , index3, index3, index3
                     );
        file->Write("\n", 1);
    }
    file->Write("\n", 1);
}

void OBJ_Write( const objModel_t* obj, const char* fileName )
{
    idFile *file = fileSystem->OpenFileWrite(fileName);

    file->Printf("# object num %d", obj->objects.Num());
    file->Write("\n", 1);

    for(int i = 0; i < obj->objects.Num(); i++)
    {
        file->Printf("o object_%d", i);
        file->Write("\n", 1);
        OBJ_WriteObject(file, obj->objects[i]);
        file->Write("\n", 1);
    }

    fileSystem->CloseFile(file);
}
