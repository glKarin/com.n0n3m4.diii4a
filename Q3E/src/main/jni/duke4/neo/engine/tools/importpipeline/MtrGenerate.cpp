// MtrGenerate.cpp
//


#pragma hdrstop

void GenerateModelMaterials_f(const idCmdArgs& args)
{
	idFileList* fileList = fileSystem->ListFilesTree("models", ".tga");

	for (int i = 0; i < fileList->GetNumFiles(); i++)
	{
		idStr file = fileList->GetFile(i);
		file.StripFileExtension();

		if (!strstr(file.c_str(), "models/"))
			continue;

		idStr folder;
		idStr temp = file.c_str() + strlen("models/");
		temp.ExtractFilePath(folder);

		if (folder.Length() == 0)
			continue;

		folder.Replace("/", "_");

		folder[folder.Length() - 1] = 0;

		idFile* f = fileSystem->OpenFileAppend(va("decls/materials/models/generated/%s.mtr", folder.c_str()));

		f->Printf("%s\n", file.c_str());
		f->Printf("{\n");
		f->Printf("\tdiffusemap %s.tga\n", file.c_str());
		f->Printf("\tbumpmap textures/common/flatnormal.tga\n");
		f->Printf("\tspecularmap textures/common/nospec.tga\n");
		f->Printf("}\n\n");
		fileSystem->CloseFile(f);
	}

	fileSystem->FreeFileList(fileList);
}

void GenerateMaterials_f(const idCmdArgs& args)
{
	idFileList *fileList = fileSystem->ListFilesTree("textures/EDF_1", ".tga");

	for (int i = 0; i < fileList->GetNumFiles(); i++)
	{
		idStr file = fileList->GetFile(i);
		file.StripFileExtension();

		if (!strstr(file.c_str(), "textures/"))
			continue;

		idStr folder;
		idStr temp = file.c_str() + strlen("textures/");		
		temp.ExtractFilePath(folder);

		if(folder.Length() == 0)
			continue;

		folder[folder.Length() - 1] = 0;

		if (folder == "common" || folder == "layout" || folder == "triggers")
			continue;

		//if (folder != "e1l1")
		//	continue;

		if(strstr(file.c_str(), "_normal"))
			continue;

		if (strstr(file.c_str(), "_spec"))
			continue;

		idFile *f = fileSystem->OpenFileAppend(va("decls/materials/generated_mtr/%s.mtr", folder.c_str()));

		f->Printf("%s\n", file.c_str());
		f->Printf("{\n");
		f->Printf("\tdiffusemap %s.tga\n", file.c_str());
		f->Printf("\tbumpmap %s_normal.tga\n", file.c_str());
		f->Printf("\tspecularmap %s_spec.tga\n", file.c_str());
		f->Printf("}\n\n");
		fileSystem->CloseFile(f);
	}

	fileSystem->FreeFileList(fileList);
}