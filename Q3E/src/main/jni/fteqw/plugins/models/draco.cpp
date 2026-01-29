#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>

	//I fucking hate c++
	typedef struct ftedracofuncs_s
	{
		void (*Release)(struct ftedracofuncs_s *ctx);	//frees all memory

		struct ftedracoattr_s
		{
			void *ptr;
			int isnormalised;
			unsigned int type;	//gl constants, because why not
			unsigned int components; //1-4
			unsigned int bytestride;
			unsigned int usage;	//typical usage type.
			unsigned int uniqueid;	//annoying extra lookup.
		} *attrib;
		unsigned int num_attribs;

		//mesh data
		char triangleloop;
		unsigned int num_indexes;
		unsigned int *ptr_indexes, num_vertexes;
	} ftedracofuncs_t;

	ftedracofuncs_t *Draco_Decode(void *ptr, size_t size);

#ifdef __cplusplus
};
#endif

#if !defined(DRACO_API_ONLY) && defined(HAVE_DRACO)
#include <draco/compression/decode.h>

struct dracofuncs_t : public ftedracofuncs_s
{
	draco::Decoder decoder;
	draco::DecoderBuffer buffer;
	draco::Mesh mesh;

	static void DoRelease(ftedracofuncs_s *ctx)
	{
		auto d = reinterpret_cast<dracofuncs_t*>(ctx);
		delete[] d->ptr_indexes;
		for (auto i = d->num_attribs; i --> 0; )
			delete[] (char*)d->attrib[i].ptr;
		delete[] d->attrib;
		delete d;	//do all the third-party destructor stuff too.
	}
	dracofuncs_t()
	{
		Release = DoRelease;
	}
};
template <class attrtype, int gltype>
static void CopyAttribute(unsigned int numpoints, draco::PointAttribute *attr, ftedracofuncs_s::ftedracoattr_s *oattr)
{
	auto nc = oattr->components;
	auto *out = new attrtype[numpoints*nc];
	for (size_t i = 0; i < numpoints; i++)
		attr->GetMappedValue(draco::PointIndex(i), &out[i*nc]);

	oattr->bytestride = sizeof(attrtype)*nc; //the output data is tightly packed.
	oattr->ptr = out;
	oattr->type = gltype;
};
ftedracofuncs_t *Draco_Decode(void *ptr, size_t size)
{
	size_t tris;
	dracofuncs_t *d = new dracofuncs_t();

	d->buffer.Init((const char *)ptr, size);
	if (!d->decoder.DecodeBufferToGeometry(&d->buffer, &d->mesh).ok())
	{	//check for corruption...
		delete d;
		return NULL;
	}

	tris = d->mesh.num_faces();
	d->num_indexes = tris*3;
	d->ptr_indexes = new unsigned int[d->num_indexes];
	while (tris --> 0)
	{
		auto &tri = d->mesh.face(draco::FaceIndex(tris));
		d->ptr_indexes[tris*3+0] = tri[0].value();
		d->ptr_indexes[tris*3+1] = tri[1].value();
		d->ptr_indexes[tris*3+2] = tri[2].value();
	}

	d->num_vertexes = d->mesh.num_points();
	d->num_attribs = d->mesh.num_attributes();
	d->attrib = new struct ftedracofuncs_s::ftedracoattr_s[d->num_attribs];
	for (unsigned int i = 0; i < d->num_attribs; i++)
	{
		draco::PointAttribute *attr = d->mesh.attribute(i);
		auto oattr = &d->attrib[i];
		oattr->isnormalised = attr->normalized();
		oattr->bytestride = attr->byte_stride();
		oattr->components = attr->num_components();
		oattr->usage = attr->attribute_type();
		oattr->uniqueid = attr->unique_id();
#define GL_BYTE					0x1400
#define GL_UNSIGNED_BYTE		0x1401
#define GL_SHORT				0x1402
#define GL_UNSIGNED_SHORT		0x1403
#define GL_INT					0x1404
#define GL_UNSIGNED_INT			0x1405
#define GL_FLOAT				0x1406
#define GL_DOUBLE				0x140A
#define GL_INT64_ARB            0x140E
#define GL_UNSIGNED_INT64_ARB   0x140F

		switch(attr->data_type())
		{
		default:
			memset(oattr, 0, sizeof(*oattr));
			break;
		case draco::DataType::DT_INT8:		CopyAttribute< int8_t,	GL_BYTE					>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_UINT8:		CopyAttribute<uint8_t,	GL_UNSIGNED_BYTE		>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_INT16:		CopyAttribute< int16_t,	GL_SHORT				>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_UINT16:	CopyAttribute<uint16_t,	GL_UNSIGNED_SHORT		>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_INT32:		CopyAttribute< int32_t,	GL_INT					>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_UINT32:	CopyAttribute<uint32_t,	GL_UNSIGNED_INT			>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_INT64:		CopyAttribute< int64_t,	GL_INT64_ARB			>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_UINT64:	CopyAttribute<uint64_t,	GL_UNSIGNED_INT64_ARB	>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_FLOAT32:	CopyAttribute<float,	GL_FLOAT				>(d->num_vertexes, attr, oattr);	break;
		case draco::DataType::DT_FLOAT64:	CopyAttribute<double,	GL_DOUBLE				>(d->num_vertexes, attr, oattr);	break;
		}
	}
	return d;
}

#endif