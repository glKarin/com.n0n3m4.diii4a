#ifndef _KARIN_POSTPROCESS_BUFFER_H
#define _KARIN_POSTPROCESS_BUFFER_H

class idFramebuffer;

// postprocess framebuffer
class sdPostprocessBuffer
{
	public:
							sdPostprocessBuffer(void);

        void				Begin(int index);
        void				End(void);
		void				Clear(void) const;
		bool				Init(int width, int height, float scale = 1.0f);
		void				Shutdown(void);
		uint32_t			GetFramebuffer(void) const;
        // framebuffer size
        int					RenderWidth(void) const {
            return width;
        }
        int					RenderHeight(void) const {
            return height;
        }
        // texture size
        int					UploadWidth(void) const;
        int					UploadHeight(void) const;
		void				ClearAll(void) const;

	private:
		void				UploadImage(void) const;

	private:
		int					width;
		int					height;
		idFramebuffer		*fb;
        idImage				*images[2];
		int					currentBuffer;

							sdPostprocessBuffer(const sdPostprocessBuffer &);
							sdPostprocessBuffer & operator=(const sdPostprocessBuffer &);
};

extern sdPostprocessBuffer postprocessBuffer;

#endif

