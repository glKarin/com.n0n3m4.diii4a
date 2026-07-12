// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLRENDERPROGRAM_H__
#define __DECLRENDERPROGRAM_H__

/*
===============================================================================

sdDeclRenderProgram

===============================================================================
*/

class sdRenderProgramShader
{
	public:
		typedef enum shaderType_e {
			ST_INVALID = 0,
			ST_VERTEX,
			ST_FRAGMENT,
		} shaderType_t;
		typedef enum shaderLang_e {
			SL_UNKNOWN = 0,
			SL_ARB,
			SL_GLSL,
			SL_CG,
			SL_HLSL,
		} shaderLang_t;

									sdRenderProgramShader(void);
		void						Init(void);
		bool						IsValid(void) const;
		bool						Parse(idParser &src);
		void						PostParse(const sdDeclRenderProgram *program);
		shaderType_t				GetType(void) const {
			return type;
		}
		shaderLang_t				GetLang(void) const {
			return lang;
		}
		const char *				GetSource(void) const {
			return source.c_str();
		}
		const idStrList &			GetPlaceholders(void) const {
			return placeholders;
		}
		const idList<const sdDeclRenderBinding *> & GetBindings(void) const {
			return bindings;
		}
		int							NumBindings(void) const {
			return bindings.Num();
		}
		const sdDeclRenderBinding * GetBinding(int i) const {
			return i >= 0 && i < bindings.Num() ? bindings[i] : NULL;
		}
		const sdDeclRenderBinding * GetBinding(const char *name) const;
		const char *				GetPlaceholder(int i) const {
			return i >= 0 && i < placeholders.Num() ? placeholders[i].c_str() : NULL;
		}
		void						ExportSource(const char *path, const char *filename, const char *name, bool raw = false) const;
		bool						HasPostprocessTexture(void) const;
		bool						HasDefine(const char *macro) const {
			return defines.FindIndex(macro) != -1;
		}

	private:
		void						HandleInclude(sdStringBuilder_Heap &buf, const char *program, const char *fileName);
		void						BuildSource(sdStringBuilder_Heap &buf, const char *program, const char *text, int length);

	private:
		shaderType_t				type;
		shaderLang_t				lang;
		idStr						sourceRaw;
		idStrList					placeholders;
		idStr						source;
		idList<const sdDeclRenderBinding *> bindings;
		idStrList					defines;

		friend class sdDeclRenderProgram;
};

// NOT use, only for parse renderprogs decl
class sdDeclRenderProgram : public idDecl {
public:
	enum {
		LOWRANGEUV = 1 << 1,
		INTERACTION = 1 << 2,
	};
									sdDeclRenderProgram();

	virtual							~sdDeclRenderProgram() {}

	// Override from idDecl
	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char* text, const int textLength );
	virtual size_t					Size( void ) const { return sizeof( sdDeclRenderProgram ); }
	virtual void					FreeData();
	const sdRenderProgramShader	*	GetVertexShader(void) const {
		return vertex.IsValid() ? &vertex : NULL;
	}
	const sdRenderProgramShader	*	GetFragmentShader(void) const {
		return fragment.IsValid() ? &fragment : NULL;
	}
	bool							IsCompleted(void) const {
		return vertex.IsValid() && fragment.IsValid();
	}
	bool							IsInteraction(void) const {
		return (flags & INTERACTION) != 0;
	}
	bool							HasPostprocess(void) const;
	int								DrawStateBits(void) const {
		return drawStateBits;
	}
	bool							HasDefine(const char *macro) const;

	void							ExportSource(const char *path, bool raw = false) const;
	static void						ExportDeclRenderPrograms_f(const idCmdArgs &args);

private:
	void							Init(void);
	bool							ParseShader(idParser &src);
	bool							ParseState(idParser &src);
	void							ParseBlend(idParser &src);
	void							ParseDepthFunc(idParser &src);
	int								NameToDstBlendMode(const idStr &name);
	int								NameToSrcBlendMode(const idStr &name);

private:
	int								flags;
	sdRenderProgramShader			vertex;
	sdRenderProgramShader			fragment;
	int								drawStateBits;
};

#endif /* !__DECLRENDERPROGRAM_H__ */
