#ifndef _KARIN_STENCIL_TEXTURE_H
#define _KARIN_STENCIL_TEXTURE_H

class idImage;
class idFramebuffer;

// Stencil texture by framebuffer
class idStencilTexture
{
	public:
		idStencilTexture();

		void Begin(void);
		void End(void);
		void Bind(void); // Stencil
		void BindDepth(void);
		void BlitStencil(void);
		bool Init(int width, int height);
		void Shutdown(void);
		// texture size
		int	 UploadWidth(void) const;
		int	 UploadHeight(void) const;
		// framebuffer size
		int	 Width(void) const {
			return width;
		}
		int	 Height(void) const {
			return height;
		}
		void BlitDepth(void);
		static bool IsAvailable(void);

	private:
		int width;
		int height;
		idFramebuffer *fb;
		idImage *depthStencilTexture;

		idStencilTexture(const idStencilTexture &);
		idStencilTexture & operator=(const idStencilTexture &);

		friend void R_CreateOfflineScreenDepthStencilTexture(idImage *image);
};

extern idStencilTexture stencilTexture;
#endif
