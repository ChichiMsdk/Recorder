#include "app.h"

Camera2D 
init_camera(float x, float y, float scale)
{
	Camera2D cam;
    cam.x = x;
    cam.y = y;
    cam.scale = scale;
	return cam;
}

void 
move_camera(Camera2D* cam, float dx, float dy)
{
    cam->x += dx;
    cam->y += dy;
}

void 
apply_camera(Camera2D* cam, SDL_Renderer *renderer)
{
	/* SDL_SetRenderScale(renderer, cam->scale, cam->scale); */
	SDL_Rect viewport = 
	{
		.x = (int)(-cam->x),
		.y = (int)(-cam->y),
		.w = (int)(g_win_w),
		.h = (int)(g_win_h),
	};
	/* SDL_SetRenderViewport(renderer, &viewport); */
}

void
zoom_camera(Camera2D *cam, float factor, int mX, int mY)
{
    Vec2f w_Before = screen_to_world(cam, (Vec2f){mX, mY});

	printf("mX: %d mY: %d\n", mX, mY);
    // Apply the zoom
    cam->scale *= factor;

    // Convert the mouse coordinates to world coordinates after zoom
    Vec2f w_After = screen_to_world(cam, (Vec2f){mX, mY});

	printf("aX: %f aY: %f\n", w_After.x, w_After.y);
    // Adjust the camera position to keep the mouse point constant

	cam->x = (w_Before.x - w_After.x);
	cam->y = (w_Before.y - w_After.y);
}

void 
set_scale_camera(Camera2D *cam, float scale)
{
	cam->scale = scale;
}

Vec2f
screen_to_world(Camera2D* cam, Vec2f screen)
{
	float wX, wY;

    wX = screen.x / cam->scale + cam->x;
    wY = screen.y / cam->scale + cam->y;
	return vec2f(wX, wY);
}
