//karin: Raven newShaderStage
#ifndef RV_NEWSHADERSTAGE_H
#define RV_NEWSHADERSTAGE_H

#define MAX_SHADER_PARMS 16
#define MAX_SHADER_TEXTURES 8

class idMaterial;
class idImage;
class idLexer;

class rvNewShaderStage
{
public:
    rvNewShaderStage();

    // parse
    bool ParseShaderParm(idLexer &src, idMaterial *material);
    bool ParseShaderTexture(idLexer &src, idMaterial *material);
    bool ParseGLSLProgram(idLexer &src, idMaterial *material);
	void ReloadImages(bool force);

    // bind
    void BindUniform(const float *regs);
    bool Bind(void);
    void UnbindUniform(void);
    void Unbind(void);

    // state
    bool IsValid(void) const {
        return shaderProgram && shaderProgram->program > 0;
    }

    template<class T>
    struct rvNewShaderStageParm {
        int index; // order, start with 0
        idStr name; // uniform variant name
        T value; // pointer type data
        int numValue; // data value count; uniform variant maybe not vec4 in GLSL shader
        GLint location; // uniform variant location
    };
    const shaderProgram_t 				            *shaderProgram;
    rvNewShaderStageParm<int[4]>					shaderParms[MAX_SHADER_PARMS];
    int					                            numShaderParms;
    rvNewShaderStageParm<idImage *> 			    shaderTextures[MAX_SHADER_TEXTURES];
    int					                            numShaderTextures;

private:
    bool LoadSource(const char *name, const char *extension, idStr &ret);
    bool LoadGLSLProgram(const char *name);

    template<class T>
    GLint GetLocation(rvNewShaderStageParm<T> *p) {
        if(p->location >= 0)
            return p->location;
        p->location = qglGetUniformLocation(shaderProgram->program, p->name.c_str());
        return p->location;
    }
};

#endif //RV_NEWSHADERSTAGE_H
