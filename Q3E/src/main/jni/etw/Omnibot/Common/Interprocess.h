
#ifndef __INTERPROCESS_H__
#define __INTERPROCESS_H__

namespace InterProcess
{
	void Init();
	void Shutdown();
	void Update();
	void Enable(bool _en);

	void DrawLine(const Vector3f &_a, const Vector3f &_b, obColor _color, float _time);
	void DrawRadius(const Vector3f &_a, float _radius, obColor _color, float _time);
	void DrawBounds(const AABB &_aabb, obColor _color, float _time, AABB::Direction _dir);
	void DrawPolygon(const Vector3List &_vertices, obColor _color, float _time);
	void DrawText(const Vector3f &_a, const char *_txt, obColor _color, float _time);
}

#endif
