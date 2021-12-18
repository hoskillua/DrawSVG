#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#define RasterDetectStep 0.2
#define Fzero 1e-20
#include "triangulation.h"

using namespace std;


namespace CMU462
{

	// Implements SoftwareRenderer //

	void SoftwareRendererImp::draw_svg(SVG& svg)
	{
		clear_sample();
		// set top level transformation
		transformation = svg_2_screen;

		// draw all elements
		for (size_t i = 0; i < svg.elements.size(); ++i)
		{
			draw_element(svg.elements[i]);
		}

		// draw canvas outline
		Vector2D a = transform(Vector2D(0, 0));
		a.x--;
		a.y--;
		Vector2D b = transform(Vector2D(svg.width, 0));
		b.x++;
		b.y--;
		Vector2D c = transform(Vector2D(0, svg.height));
		c.x--;
		c.y++;
		Vector2D d = transform(Vector2D(svg.width, svg.height));
		d.x++;
		d.y++;

		rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
		rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
		rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
		rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

		// resolve and send to render target
		resolve();
	}

	void SoftwareRendererImp::set_sample_rate(size_t sample_rate)
	{

		// Task 4:
		// You may want to modify this for supersampling support
		this->sample_rate = sample_rate;
	}

	void SoftwareRendererImp::set_render_target(unsigned char* render_target,
		size_t width, size_t height)
	{

		// Task 4:
		// You may want to modify this for supersampling support

		this->render_target = render_target;
		this->target_w = width;
		this->target_h = height;
	}

	void SoftwareRendererImp::draw_element(SVGElement* element)
	{

		// Task 5 (part 1):
		// Modify this to implement the transformation stackS
		Matrix3x3 Temp = transformation;
		transformation = transformation * element->transform;
		switch (element->type)
		{
		case POINT:
			draw_point(static_cast<Point&>(*element));
			break;
		case LINE:
			draw_line(static_cast<Line&>(*element));
			break;
		case POLYLINE:
			draw_polyline(static_cast<Polyline&>(*element));
			break;
		case RECT:
			draw_rect(static_cast<Rect&>(*element));
			break;
		case POLYGON:
			draw_polygon(static_cast<Polygon&>(*element));
			break;
		case ELLIPSE:
			draw_ellipse(static_cast<Ellipse&>(*element));
			break;
		case IMAGE:
			draw_image(static_cast<Image&>(*element));
			break;
		case GROUP:
			draw_group(static_cast<Group&>(*element));
			break;
		default:
			break;
		}
		transformation = Temp;
	}

	// Primitive Drawing //

	void SoftwareRendererImp::draw_point(Point& point)
	{

		Vector2D p = transform(point.position);
		rasterize_point(p.x, p.y, point.style.fillColor);
	}

	void SoftwareRendererImp::draw_line(Line& line)
	{

		Vector2D p0 = transform(line.from);
		Vector2D p1 = transform(line.to);
		rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);
	}

	void SoftwareRendererImp::draw_polyline(Polyline& polyline)
	{

		Color c = polyline.style.strokeColor;

		if (c.a != 0)
		{
			int nPoints = polyline.points.size();
			for (int i = 0; i < nPoints - 1; i++)
			{
				Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
				Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
				rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
			}
		}
	}

	void SoftwareRendererImp::draw_rect(Rect& rect)
	{

		Color c;

		// draw as two triangles
		float x = rect.position.x;
		float y = rect.position.y;
		float w = rect.dimension.x;
		float h = rect.dimension.y;

		Vector2D p0 = transform(Vector2D(x, y));
		Vector2D p1 = transform(Vector2D(x + w, y));
		Vector2D p2 = transform(Vector2D(x, y + h));
		Vector2D p3 = transform(Vector2D(x + w, y + h));

		// draw fill
		c = rect.style.fillColor;
		if (c.a != 0)
		{
			rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
			rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
		}

		// draw outline
		c = rect.style.strokeColor;
		if (c.a != 0)
		{
			rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
			rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
			rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
			rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
		}
	}

	void SoftwareRendererImp::draw_polygon(Polygon& polygon)
	{

		Color c;

		// draw fill
		c = polygon.style.fillColor;
		if (c.a != 0)
		{

			// triangulate
			vector<Vector2D> triangles;
			triangulate(polygon, triangles);

			// draw as triangles
			for (size_t i = 0; i < triangles.size(); i += 3)
			{
				Vector2D p0 = transform(triangles[i + 0]);
				Vector2D p1 = transform(triangles[i + 1]);
				Vector2D p2 = transform(triangles[i + 2]);
				rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
			}
		}

		// draw outline
		c = polygon.style.strokeColor;
		if (c.a != 0)
		{
			int nPoints = polygon.points.size();
			for (int i = 0; i < nPoints; i++)
			{
				Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
				Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
				rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
			}
		}
	}

	void SoftwareRendererImp::draw_ellipse(Ellipse& ellipse)
	{

		// Extra credit
	}

	void SoftwareRendererImp::draw_image(Image& image)
	{

		Vector2D p0 = transform(image.position);
		Vector2D p1 = transform(image.position + image.dimension);

		rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
	}

	void SoftwareRendererImp::draw_group(Group& group)
	{

		for (size_t i = 0; i < group.elements.size(); ++i)
		{
			draw_element(group.elements[i]);
		}
	}

	// Rasterization //

	// The input arguments in the rasterization functions
	// below are all defined in screen space coordinates

	void SoftwareRendererImp::set_sample_buffer(int x, int y, Color color)
	{
		// check bounds
		if (x < 0 || x >= target_w * sample_rate)
			return;
		if (y < 0 || y >= target_h * sample_rate)
			return;

		// fill sample - NOT doing alpha blending!
		sample_buffer[4 * (x + y  * target_w * sample_rate)] =
			color.a * (color.r * 255) +
			(1 - color.a) * sample_buffer[4 * (x + y * target_w * sample_rate)];
		sample_buffer[4 * (x + y * target_w * sample_rate) + 1] =
			color.a * (color.g * 255) +
			(1 - color.a) * sample_buffer[4 * (x + y * target_w * sample_rate) + 1];
		sample_buffer[4 * (x + y * target_w * sample_rate) + 2] =
			color.a * (color.b * 255) +
			(1 - color.a) * sample_buffer[4 * (x + y * target_w * sample_rate) + 2];
		sample_buffer[4 * (x + y * target_w * sample_rate) + 3] =
			(color.a * 255) +
			(1 - color.a) * sample_buffer[4 * (x + y * target_w * sample_rate) + 3];
	}

	void SoftwareRendererImp::rasterize_point(float x, float y, Color color)
	{

		// fill in the nearest pixel
		int sx = (int)floor(x);
		int sy = (int)floor(y);

		// check bounds
		if (sx < 0 || sx >= target_w)
			return;
		if (sy < 0 || sy >= target_h)
			return;

		// fill sample - NOT doing alpha blending!
		for (int i = 0; i < sample_rate; i++)
			for (int j = 0; j < sample_rate; j++)
			{
				sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate)] =
					color.a * (uint8_t)(color.r * 255) +
					(1 - color.a) * sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate)];
				sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 1] =
					color.a * (uint8_t)(color.g * 255) +
					(1 - color.a) * sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 1];
				sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 2] =
					color.a * (uint8_t)(color.b * 255) +
					(1 - color.a) * sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 2];
				sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 3] =
					color.a * (uint8_t)(color.a * 255) +
					(1 - color.a) * sample_buffer[4 * ((sx * sample_rate + i) + (sy * sample_rate + j) * target_w * sample_rate) + 3];
			}
	}

	void SoftwareRendererImp::rasterize_line(float x0, float y0,
		float x1, float y1,
		Color color)
	{


		bool antialising = false;
		float swidth = 0.6;
		float ewidth = 0.6;

		float m = (y1 - y0) / (x1 - x0);
		if (swidth < 0.5 && ewidth < 0.5)
			return;
		else if (swidth > 1.1 || ewidth > 1.1 || sample_rate > 1)
		{
			if (x0 > x1)
			{
				swap(y0, y1);
				swap(x0, x1);
			}

			float x00, x01, x10, x11, y00, y01, y10, y11;

			float theta = atan2(y1 - y0, x1 - x0);
			x00 = x0 + sin(theta) * swidth;
			y00 = y0 - cos(theta) * swidth;
			x01 = x0 - sin(theta) * swidth;
			y01 = y0 + cos(theta) * swidth;
			x10 = x1 + sin(theta) * ewidth;
			y10 = y1 - cos(theta) * ewidth;
			x11 = x1 - sin(theta) * ewidth;
			y11 = y1 + cos(theta) * ewidth;

			rasterize_triangle(x00, y00, x01, y01, x10, y10, color);
			rasterize_triangle(x11, y11, x01, y01, x10, y10, color);
		}
		else
		{
			///Xiaolin Wu's line algorithm  if anti-aliasing
			///Bresenham's line algorithm   else
			//(O(max(abs(x1-x0), abs(y1-y0))))

			//if y changes faster than x, we iterate over y and change x by 1/m
			if (m > 1 || m < -1)
			{
				m = 1 / m;
				//make sure we iterate from bottom up
				if (y0 > y1)
				{
					swap(y0, y1);
					swap(x0, x1);
				}
				while (y0 < y1)
				{
					if (antialising)
					{
						float x = floor(x0) + 0.5;
						if (x0 > x)
						{
							color.a = x0 - x;
							rasterize_point(x0 + 1, y0, color);
							color.a = 1 - color.a;
							rasterize_point(x0, y0, color);
						}
						else
						{
							color.a = x - x0;
							rasterize_point(x0 - 1, y0, color);
							color.a = 1 - color.a;
							rasterize_point(x0, y0, color);
						}
					}
					else
						rasterize_point(x0, y0, color);
					x0 += m;
					y0++;
				}
			}
			//if x changes faster than y, we iterate over x and change y by m
			else
			{
				//make sure we iterate from left to right
				if (x0 > x1)
				{
					swap(y0, y1);
					swap(x0, x1);
				}
				while (x0 < x1)
				{
					if (antialising)
					{
						float y = floor(y0) + 0.5;
						if (y0 > y)
						{
							color.a = y0 - y;
							rasterize_point(x0, y0 + 1, color);
							color.a = 1 - color.a;
							rasterize_point(x0, y0, color);
						}
						else
						{
							color.a = y - y0;
							rasterize_point(x0, y0 - 1, color);
							color.a = 1 - color.a;
							rasterize_point(x0, y0, color);
						}
					}
					else
						rasterize_point(x0, y0, color);
					y0 += m;
					x0++;
				}
			}
		}
	}

	bool SoftwareRendererImp::point_in_traingle(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float i, float j)
	{
		float step = 0.5 / sample_rate;
		float p1 = (i + step - x0) * (y1 - y0) - (j + step - y0) * (x1 - x0);
		float p2 = (i + step - x1) * (y2 - y1) - (j + step - y1) * (x2 - x1);
		float p3 = (i + step - x2) * (y0 - y2) - (j + step - y2) * (x0 - x2);
		return (p1 > -Fzero && p2 > -Fzero && p3 > -Fzero) || (p1 <= Fzero && p2 <= Fzero && p3 <= Fzero);
	}

	void SoftwareRendererImp::filldivision(
		Color color,
		int xmin, int ymin,
		int xmax, int ymax)
	{
		for (int i = (xmin)*sample_rate; i < (xmax + 1) * sample_rate; i++)
		{
			for (int j = (ymin)*sample_rate; j < (ymax + 1) * sample_rate; j++)
			{
				set_sample_buffer(i, j, color);
				/*sample_buffer[4 * (i + j * target_w * sample_rate)] = (uint8_t)(color.r * 255);
				sample_buffer[4 * (i + j * target_w * sample_rate) + 1] = (uint8_t)(color.g * 255);
				sample_buffer[4 * (i + j * target_w * sample_rate) + 2] = (uint8_t)(color.b * 255);
				sample_buffer[4 * (i + j * target_w * sample_rate) + 3] = (uint8_t)(color.a * 255);*/
			}
		}
	}

	bool SoftwareRendererImp::TriangleRectFill(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float x00, float y00,
		float x11, float y11)
	{

		return point_in_traingle(x0, y0, x1, y1, x2, y2, x00 - 0.5 / sample_rate, y00 - 0.5 / sample_rate)
			&& point_in_traingle(x0, y0, x1, y1, x2, y2, x00 - 0.5 / sample_rate, y11 - 0.5 / sample_rate)
			&& point_in_traingle(x0, y0, x1, y1, x2, y2, x11 - 0.5 / sample_rate, y00 - 0.5 / sample_rate)
			&& point_in_traingle(x0, y0, x1, y1, x2, y2, x11 - 0.5 / sample_rate, y11 - 0.5 / sample_rate);
	}

	bool SoftwareRendererImp::LineIntersectH(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float x3, float y3)
	{
		if (abs((y0 - y1)) * 200 < abs(x0 - x1))
			return false;
		else if (abs((x1 - x0)) * 200 < abs(y1 - y0))
			return (x2 <= x0 && x0 <= x3);

		float m0 = (y1 - y0) / (x1 - x0);
		float c0 = y0 - m0 * x0;
		// m0 x + c0
		float c2 = y2;
		// m2 x + c2
		float x = (c2 - c0) / (m0);

		return (min(x0, x1) <= x && x <= max(x0, x1)
			&& x2 <= x && x <= x3);
	}
	bool SoftwareRendererImp::LineIntersectV(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float x3, float y3)
	{
		if (abs((x1 - x0)) * 200 < abs(y1 - y0))
			return false;
		else if (abs((y0 - y1)) * 200 < abs(x0 - x1))
			return (y2 <= y0 && y0 <= y3);
		/// YET
		float m0 = (x1 - x0) / (y1 - y0);
		float c0 = x0 - m0 * y0;
		// m0 y + c0
		float c2 = x2;
		// c2
		float y = (c2 - c0) / (m0);

		return (min(y0, y1) <= y && y <= max(y0, y1)
			&& y2 <= y && y <= y3);
	}


	bool SoftwareRendererImp::TriangleRectIntersect(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float x00, float y00,
		float x11, float y11)
	{
		return LineIntersectV(x0, y0, x1, y1, x00, y00, x00, y11)
			|| LineIntersectH(x0, y0, x1, y1, x00, y00, x11, y00)
			|| LineIntersectH(x0, y0, x1, y1, x00, y11, x11, y11)
			|| LineIntersectV(x0, y0, x1, y1, x11, y00, x11, y11)
			|| LineIntersectV(x2, y2, x1, y1, x00, y00, x00, y11)
			|| LineIntersectH(x2, y2, x1, y1, x00, y00, x11, y00)
			|| LineIntersectH(x2, y2, x1, y1, x00, y11, x11, y11)
			|| LineIntersectV(x2, y2, x1, y1, x11, y00, x11, y11)
			|| LineIntersectV(x2, y2, x0, y0, x00, y00, x00, y11)
			|| LineIntersectH(x2, y2, x0, y0, x00, y00, x11, y00)
			|| LineIntersectH(x2, y2, x0, y0, x00, y11, x11, y11)
			|| LineIntersectV(x2, y2, x0, y0, x11, y00, x11, y11)
			|| (x00 <= x0 && x0 <= x11 && y00 <= y0 && y0 <= y11
				&& x00 <= x1 && x1 <= x11 && y00 <= y1 && y1 <= y11
				&& x00 <= x2 && x2 <= x11 && y00 <= y2 && y2 <= y11);

	}

	void SoftwareRendererImp::divide_screen2x2_rasterize_tr(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		Color color,
		int xmin, int ymin,
		int xmax, int ymax, float threshold)
	{
		if ((xmax - xmin) * (ymax - ymin) <= threshold * threshold)
		{
			if (/*!(rand() % 1000) || */1)
			{
				for (int i = (xmin - 1) * sample_rate; i < (xmax + 1) * sample_rate; i++)
				{
					for (int j = (ymin - 1) * sample_rate; j < (ymax + 1) * sample_rate; j++)
					{
						if (point_in_traingle(x0, y0, x1, y1, x2, y2, i / ((float)sample_rate), j / ((float)sample_rate)))
						{
							set_sample_buffer(i, j, color);
						}
					}
				}
			}

		}
		else
		{
			if (xmax - xmin > ymax - ymin)
			{
				float xmid = (xmax + xmin) / 2;

				if (TriangleRectFill(x0, y0, x1, y1, x2, y2, xmin, ymin, xmid, ymax))
					filldivision(color, xmin, ymin, xmid, ymax);
				else if (TriangleRectIntersect(x0, y0, x1, y1, x2, y2, xmin, ymin, xmid, ymax))
					divide_screen2x2_rasterize_tr(x0, y0, x1, y1, x2, y2, color, xmin, ymin, xmid, ymax, threshold);

				if (TriangleRectFill(x0, y0, x1, y1, x2, y2, xmid + 1, ymin, xmax, ymax))
					filldivision(color, xmid + 1, ymin, xmax, ymax);
				else if (TriangleRectIntersect(x0, y0, x1, y1, x2, y2, xmid + 1, ymin, xmax, ymax))
					divide_screen2x2_rasterize_tr(x0, y0, x1, y1, x2, y2, color, xmid + 1, ymin, xmax, ymax, threshold);

			}
			else
			{
				float ymid = (ymax + ymin) / 2;

				if (TriangleRectFill(x0, y0, x1, y1, x2, y2, xmin, ymin, xmax, ymid))
					filldivision(color, xmin, ymin, xmax, ymid);
				else if (TriangleRectIntersect(x0, y0, x1, y1, x2, y2, xmin, ymin, xmax, ymid))
					divide_screen2x2_rasterize_tr(x0, y0, x1, y1, x2, y2, color, xmin, ymin, xmax, ymid, threshold);

				if (TriangleRectFill(x0, y0, x1, y1, x2, y2, xmin, ymid + 1, xmax, ymax))
					filldivision(color, xmin, ymid + 1, xmax, ymax);
				else if (TriangleRectIntersect(x0, y0, x1, y1, x2, y2, xmin, ymid + 1, xmax, ymax))
					divide_screen2x2_rasterize_tr(x0, y0, x1, y1, x2, y2, color, xmin, ymid + 1, xmax, ymax, threshold);

			}
		}
	}



	void SoftwareRendererImp::rasterize_triangle(float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		Color color)
	{
		//cout << x0 << ", " << y0 << "	" << x1 << ", " << y1 << "	" << x2 << ", " << y2 << endl;

		float xmin, ymin, xmax, ymax;
		xmin = max(min(x0, min(x1, x2)) - 0.5f, 0.01f);
		ymin = max(min(y0, min(y1, y2)) - 0.5f, 0.01f);
		xmax = min(max(x0, max(x1, x2)) + 0.5f, target_w + 0.01f);
		ymax = min(max(y0, max(y1, y2)) + 0.5f, target_h + 0.01f);

		divide_screen2x2_rasterize_tr(x0, y0, x1, y1, x2, y2, color, xmin, ymin, xmax, ymax, 16);


		/*for (int i = (xmin); i < (xmax); i++)
		{
			for (int j = (ymin); j < (ymax); j++)
			{
				float p1 = (i + 0.5f - x0) * (y1 - y0) - (j + 0.5f - y0) * (x1 - x0);
				float p2 = (i + 0.5f - x1) * (y2 - y1) - (j + 0.5f - y1) * (x2 - x1);
				float p3 = (i + 0.5f - x2) * (y0 - y2) - (j + 0.5f - y2) * (x0 - x2);
				if ((p1 >= -Fzero && p2 >= -Fzero && p3 >= -Fzero) || (p1 <= Fzero && p2 <= Fzero && p3 <= Fzero))
				{
					render_target[4 * (i + j * target_w)] = (uint8_t)(color.r * 255);
					render_target[4 * (i + j * target_w) + 1] = (uint8_t)(color.g * 255);
					render_target[4 * (i + j * target_w) + 2] = (uint8_t)(color.b * 255);
					render_target[4 * (i + j * target_w) + 3] = (uint8_t)(color.a * 255);
				}
			}
		}*/

	}

	void SoftwareRendererImp::rasterize_image(float x0, float y0,
		float x1, float y1,
		Texture& tex)
	{
		//if (sampler->get_sample_method() == BILINEAR)
		//	for (int i = max(x0, .0f); i < min(x1, (float)target_w); i++)
		//		for (int j = max(y0, .0f); j < min(y1, (float)target_h); j++)
		//			rasterize_point(i, j, sampler->sample_bilinear(tex, (i - x0 + 0.5f) / (x1 - x0), (j - y0 + 0.5f) / (y1 - y0), 0));
		//else if (sampler->get_sample_method() == TRILINEAR)
		//{
		float L = sqrt(tex.width * tex.height / (x1 - x0) / (y1 - y0));
		for (int i = max(x0, .0f); i < min(x1, (float)target_w); i++)
			for (int j = max(y0, .0f); j < min(y1, (float)target_h); j++)
			{
				if (L > 1)
					rasterize_point(i, j, sampler->
						sample_trilinear(tex, (i - x0 + 0.5f) / (x1 - x0), (j - y0 + 0.5f) / (y1 - y0), L, L));
				else
					rasterize_point(i, j, sampler->
						sample_bilinear(tex, (i - x0 + 0.5f) / (x1 - x0), (j - y0 + 0.5f) / (y1 - y0)));
			}

		/*}*/
	}

	// resolve samples to render target
	void SoftwareRendererImp::resolve(void)
	{
		//clear_target();
		//cout << target_w << "x" << target_h << ":" << sample_rate << ", " << tricount << endl;
		for (int x = 0; x < target_w; x++)
			for (int y = 0; y < target_h; y++)
			{
				float r = 0, g = 0, b = 0, a = 0;
				for (int i = 0; i < sample_rate; i++)
					for (int j = 0; j < sample_rate; j++)
					{
						r += sample_buffer[4 * ((x * sample_rate + i) + (y * sample_rate + j) * target_w * sample_rate)];
						g += sample_buffer[4 * ((x * sample_rate + i) + (y * sample_rate + j) * target_w * sample_rate) + 1];
						b += sample_buffer[4 * ((x * sample_rate + i) + (y * sample_rate + j) * target_w * sample_rate) + 2];
						a += sample_buffer[4 * ((x * sample_rate + i) + (y * sample_rate + j) * target_w * sample_rate) + 3];
					}
				a = a / sample_rate / sample_rate;
				render_target[4 * (x + y * target_w)    ] = (uint8_t)min((r /sample_rate / sample_rate * (255 / a)),255.f);
				render_target[4 * (x + y * target_w) + 1] = (uint8_t)min((g /sample_rate / sample_rate * (255 / a)), 255.f);
				render_target[4 * (x + y * target_w) + 2] = (uint8_t)min((b /sample_rate / sample_rate * (255 / a)), 255.f);
				render_target[4 * (x + y * target_w) + 3] = (uint8_t)(255);
			}

		// Task 4:
		// Implement supersampling
		// You may also need to modify other functions marked with "Task 4".
		return;
	}

} // namespace CMU462
