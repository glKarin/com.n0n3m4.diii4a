float() CSQC_Parse_TempEntity =
{
	vector org;
	float te = readbyte();
print(sprintf("CSQC_Parse_TempEntity(%g)\n", te));
	if (te == 130)
	{
		org_x = readcoord();
		org_y = readcoord();
		org_z = readcoord();
		te_gunshot(org, 1);
	}
	else
		return FALSE;
	return TRUE;
};
