float resultfile;
void(string s) putresultstring =
{
	fputs(resultfile, s);
};

void(string s) FoundExtension =
{
	float descfile;
	string descfilename;

	putresultstring("//");putresultstring(s);putresultstring("\r\n");

	descfilename = strcat("ext/", s, ".qc");
	descfile = fopen(descfilename, 0);

	if (descfile>=0)
	{
		for(;;)
		{
			s = fgets(descfile);
			if (s) {} else break;

			putresultstring(s);
			putresultstring("\r\n");
		}
		fclose(descfile);
		putresultstring("\r\n");
	}
	else
	{
		putresultstring("//FIXME: AutoExt: No information\r\n");
		putresultstring("\r\n");	//and a blank line
	}
};

void(string inname) decompose =
{
	float in;
	float out;
	float len;
	string s;
	string outname;

	in = fopen(inname, 0);
	if (in < 0)
	{
		return;
	}

	for(;;)
	{
		s = fgets(in);
		if (s) {} else break;

		if (s == "")	//skip extra whitespace
			continue;

		len = strlen(s);
		s = substring(s, 2, len-2);
		outname = strcat("ext/", s, ".qc");
		out = fopen(outname, 2);
		while((s = fgets(in)) != "")
		{
			fputs(out, s);
			fputs(out, "\r\n");
		}
		fclose(out);
	}

	fclose(in);
};

void() worldspawn =
{
	float extlist;
	string s;

	if (!cvar("pr_checkextension"))
		error("Engine doesn't support any extensions\n");

	if (!checkextension("FRIK_FILE"))
		error("Unable to continue without FRIK_FILE\n");


//	decompose("lists/betwix.qc");
//	decompose("lists/dpextensions.qc");

	extlist = fopen("lists/extlist.txt", 0);
	resultfile = fopen("results.qc", 2);

	for(;;)
	{
		s = fgets(extlist);
		if (s) {} else break;

		if (checkextension(s))
			FoundExtension(s);
	}

	fclose(resultfile);
	fclose(extlist);

//	dprint("\n\n\n\n\n\n\n\n");
	error("autoext compleate\n");
};