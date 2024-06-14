#ifndef _KARIN_OFFLINE_SCREEN_RENDERER_H
#define _KARIN_OFFLINE_SCREEN_RENDERER_H

class idImage;
class idFramebuffer;

// Second render framebuffer
class idOfflineScreenRenderer
{
	public:
		idOfflineScreenRenderer();
		virtual ~idOfflineScreenRenderer();

		void Bind(void);
		void Unbind(void);
		void Blit(void);
		bool Init(int width, int height);
		void Shutdown(void);
		uint32_t GetFramebuffer(void) const;

	private:
		int width;
		int height;
		idFramebuffer *fb;

		idOfflineScreenRenderer(const idOfflineScreenRenderer &);
		idOfflineScreenRenderer & operator=(const idOfflineScreenRenderer &);
};

// Stencil texture by framebuffer
class idStencilTexture
{
	public:
		idStencilTexture();
		virtual ~idStencilTexture();

		void Bind(void);
		void Unbind(void);
		void Select(void);
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

// extern idOfflineScreenRenderer offlineScreenRenderer;
extern idStencilTexture stencilTexture;
#endif
