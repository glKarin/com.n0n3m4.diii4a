// PlaneImport.cpp
//



struct PlaneGeometry_t
{
	idStr shaderName;
	idList<idVec3> verts;
	idList<idVec2> st;
};

void ParseGeometryPlane(idParser& parser, PlaneGeometry_t& geometry)
{
	idToken token;

	parser.ReadToken(&token);
	geometry.shaderName = token;

	parser.ExpectTokenString("{");

	while (true)
	{
		idToken token;

		if (!parser.ReadToken(&token))
		{
			common->Error("Unexpected EOF in Geometry plane!");
		}

		if (token == "}")
			break;

		if (token == "xyz")
		{
			idVec3 vert;

			vert.x = parser.ParseFloat();
			vert.y = parser.ParseFloat();
			vert.z = parser.ParseFloat();

			idVec3 fv = vert.ConvertToIdSpace();
			fv.y = -fv.y;

			geometry.verts.Append(fv);
		}
		else if (token == "st")
		{
			idVec2 st;

			st.x = parser.ParseFloat();
			st.y = parser.ParseFloat();

			geometry.st.Append(st);
		}
		else
		{
			common->Error("Unexpected token %s\n", token.c_str());
		}
	}
}

void ImportPlane_f(const idCmdArgs& args)
{
	idList<PlaneGeometry_t> geometry;

	if (args.Argc() != 2)
	{
		common->Warning("usage: importplane <planefile>");
		return;
	}	

	char* planeFileBuffer;
	int fileLen = fileSystem->ReadFile(va("maps/%s.planes", args.Argv(1)), (void **) & planeFileBuffer);

	if (fileLen <= 0) {
		common->Warning("Failed to open file!");
		return;
	}

	idParser parser(planeFileBuffer, fileLen, args.Argv(1), LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);

	while (true)
	{
		idToken token;

		if (!parser.ReadToken(&token))
		{
			break;
		}

		if (token == "Plane")
		{
			PlaneGeometry_t newGeometry;			
			ParseGeometryPlane(parser, newGeometry);

			geometry.Append(newGeometry);
		}
		else
		{
			common->Error("Unexpected token %s\n", token.c_str());
		}
	}

	// Write out all of the obj files.
	for (int i = 0; i < geometry.Num(); i++)
	{
		PlaneGeometry_t geo = geometry[i];
		idFile* f = fileSystem->OpenFileWrite(va("maps/%s/plane_%d.obj", args.Argv(1), i));

		for (int d = 0; d < geo.verts.Num(); d++)
		{
			f->Printf("v %f %f %f\n", geo.verts[d].x, geo.verts[d].y, geo.verts[d].z);
		}

		for (int d = 0; d < geo.verts.Num(); d++)
		{
			f->Printf("vt %f %f\n", geo.st[d].x, geo.st[d].y);
		}

		for (int d = 0; d < geo.verts.Num(); d += 3)
		{
			int index0 = d + 0 + 1;
			int index1 = d + 1 + 1;
			int index2 = d + 2 + 1;

			f->Printf("f %d/%d %d/%d %d/%d\n", index0, index0, index1, index1, index2, index2);
		}

		fileSystem->CloseFile(f);
	}


	idFile* f = fileSystem->OpenFileWrite(va("maps/%s.entities", args.Argv(1)));

	for (int i = 0; i < geometry.Num(); i++) 
	{
		f->Printf("// entity %d\n", i);
		f->Printf("{\n");
		f->Printf("\"classname\" \"func_static\"\n");
		f->Printf("\"model\" \"%s\"\n", va("maps/%s/plane_%d.obj", args.Argv(1), i));
		f->Printf("\"name\" \"func_static_%d\"\n", i);
		f->Printf("\"inline\" \"1\"\n");
		f->Printf("\"origin\" \"0 0 0\"\n");
		f->Printf("\"forceshader\" \"%s\"\n", geometry[i].shaderName.c_str());
		f->Printf("}\n");
	}
		
	fileSystem->CloseFile(f);
}