#ifndef _KARIN_DEPTH_STENCIL_RENDERER_H
#define _KARIN_DEPTH_STENCIL_RENDERER_H

class idFramebuffer;

// Depth/stencil framebuffer
class idDepthStencilRenderer
{
	public:
		idDepthStencilRenderer();

        void Begin(void);
        void BeginBlit(void);
        void BeginRender(void);
        void End(void);
        void BindStencil(void);
        void BindDepth(void);
        void Unbind(void);
		void Blit(GLint mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		bool Init(int width, int height);
		void Shutdown(void);
		uint32_t GetFramebuffer(void) const;
        // framebuffer size
        int	 Width(void) const {
            return width;
        }
        int	 Height(void) const {
            return height;
        }
        // texture size
        int	 UploadWidth(void) const;
        int	 UploadHeight(void) const;

        static bool IsAvailable(void);

	private:
		int width;
		int height;
		idFramebuffer *fb;
        idImage *depthStencilTexture;

		idDepthStencilRenderer(const idDepthStencilRenderer &);
		idDepthStencilRenderer & operator=(const idDepthStencilRenderer &);

        friend void R_CreateOfflineScreenDepthStencilTexture(idImage *image);
};

extern idDepthStencilRenderer depthStencilRenderer;

#endif
