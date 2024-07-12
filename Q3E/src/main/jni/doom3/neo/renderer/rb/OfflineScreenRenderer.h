#ifndef _KARIN_OFFLINE_SCREEN_RENDERER_H
#define _KARIN_OFFLINE_SCREEN_RENDERER_H

class idFramebuffer;

// Secondary render framebuffer
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

// extern idOfflineScreenRenderer offlineScreenRenderer;

#endif
