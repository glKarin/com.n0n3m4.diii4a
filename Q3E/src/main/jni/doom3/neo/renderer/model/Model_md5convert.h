#ifndef _MODEL_MD5CONVERT_H
#define _MODEL_MD5CONVERT_H

namespace md5model
{
    typedef struct md5ConvertDef_s
    {
        const idDeclEntityDef *def;
        float scale;
        bool addOrigin;
        idStr mesh;
        idStrList anims;
    } md5ConvertDef_t;
};

bool R_Model_ConvertToMd5(const char *fileName);

#ifdef _MODEL_PSK
bool R_Model_HandlePskPsa(const md5model::md5ConvertDef_t &convert);
#endif

#endif
