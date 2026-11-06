$if r_detailTexture
TEMP	_DTLR0;
SUB		_DTLR0, $texCoordAttrib.xyxy, 0.5;
MUL		result.texcoord[7].zw, _DTLR0, $megaDetailTextureParams.x;
$endif