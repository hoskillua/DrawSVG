#ifndef CMU462_SOFTWARE_RENDERER_H
#define CMU462_SOFTWARE_RENDERER_H

#include <stdio.h>
#include <vector>
#include <bitset>

#include "CMU462.h"
#include "texture.h"
#include "svg_renderer.h"

namespace CMU462 { // CMU462

class SoftwareRenderer : public SVGRenderer {
 public:

  SoftwareRenderer( ) : sample_rate (1) { }

  // Free used resources
  virtual ~SoftwareRenderer( ) { }

  // Draw an svg input to render target
  virtual void draw_svg( SVG& svg ) = 0;

  // Set sample rate
  virtual void set_sample_rate( size_t sample_rate ) = 0;
  
  // Set render target
  virtual void set_render_target( unsigned char* render_target,
                                  size_t width, size_t height ) = 0;

  // Clear render target
  inline void clear_target() {
    memset(render_target, 255, 4 * target_w * target_h);

  }

  // Set texture sampler
  inline void set_tex_sampler( Sampler2D* sampler ) {
    this->sampler = sampler;
  }

  // Set svg to screen transformation
  inline void set_svg_2_screen( Matrix3x3 svg_2_screen ) {
    this->svg_2_screen = svg_2_screen;
  }

 protected:

  // Sample rate (square root of samples per pixel)
  size_t sample_rate;

  // Render target memory location
  unsigned char* render_target; 

  // Target buffer dimension (in pixels)
  size_t target_w; size_t target_h;

  // Texture sampler being used
  Sampler2D* sampler;

  // SVG coordinates to screen space coordinates
  Matrix3x3 svg_2_screen;

}; // class SoftwareRenderer


class SoftwareRendererImp : public SoftwareRenderer {
 public:

  SoftwareRendererImp( ) : SoftwareRenderer( ) { }

  // draw an svg input to render target
  void draw_svg( SVG& svg );

  // set sample rate
  void set_sample_rate( size_t sample_rate );
  
  // set render target
  void set_render_target( unsigned char* target_buffer,
                          size_t width, size_t height );

 private:

  // Primitive Drawing //

  // Draws an SVG element
  void draw_element( SVGElement* element );

  // Draws a point
  void draw_point( Point& p );

  // Draw a line
  void draw_line( Line& line );

  // Draw a polyline
  void draw_polyline( Polyline& polyline );

  // Draw a rectangle
  void draw_rect ( Rect& rect );

  // Draw a polygon
  void draw_polygon( Polygon& polygon );

  // Draw a ellipse
  void draw_ellipse( Ellipse& ellipse );

  // Draws a bitmap image
  void draw_image( Image& image );

  // Draw a group
  void draw_group( Group& group );

  // Rasterization //

  void set_sample_buffer(int x, int y, Color color);
  
  // rasterize a point
  void rasterize_point( float x, float y, Color color );

  // rasterize a line
  void rasterize_line( float x0, float y0,
                       float x1, float y1,
                       Color color);

  void divide_screen2x2_rasterize_tr(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      Color color,
      int xmin, int ymin,
      int xmax, int ymax, float threshold);

  bool point_in_traingle(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      float i, float j);

  bool TriangleRectIntersect(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      float x00, float y00,
      float x11, float y11);

  bool LineIntersectH(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      float x3, float y3);
  bool LineIntersectV(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      float x3, float y3);

  bool TriangleRectFill(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      float x00, float y00,
      float x11, float y11);

  void filldivision(
      Color color,
      int xmin, int ymin,
      int xmax, int ymax);

  // rasterize a triangle
  void rasterize_triangle( float x0, float y0,
                           float x1, float y1,
                           float x2, float y2,
                           Color color );


  // rasterize an image
  void rasterize_image( float x0, float y0,
                        float x1, float y1,
                        Texture& tex );

  // resolve samples to render target
  void resolve( void );

  uint8_t sample_buffer[4*16*2000*1000]; int w; int h;

  inline void clear_sample() {
    memset(sample_buffer, 255, 4 * 16 * 2000 * 1000);
  }

}; // class SoftwareRendererImp


class SoftwareRendererRef : public SoftwareRenderer {
 public:

  SoftwareRendererRef( ) : SoftwareRenderer( ) { }

  // draw an svg input to render target
  void draw_svg( SVG& svg );

  // set sample rate
  void set_sample_rate( size_t sample_rate );
  
  // set render target
  void set_render_target( unsigned char* target_buffer,
                          size_t width, size_t height );

 private:

  // Primitive Drawing //

  // Draws an SVG element
  void draw_element( SVGElement* element );

  // Draws a point
  void draw_point( Point& p );

  // Draw a line
  void draw_line( Line& line );

  // Draw a polyline
  void draw_polyline( Polyline& polyline );

  // Draw a rectangle
  void draw_rect ( Rect& rect );

  // Draw a polygon
  void draw_polygon( Polygon& polygon );

  // Draw a ellipse
  void draw_ellipse( Ellipse& ellipse );

  // Draws a bitmap image
  void draw_image( Image& image );

  // Draw a group
  void draw_group( Group& group );

  // Rasterization //

  // rasterize a point
  void rasterize_point( float x, float y, Color color );

  // rasterize a line
  void rasterize_line( float x0, float y0,
                       float x1, float y1,
                       Color color);

  //
  void divide_screen2x2_rasterize_tr(float x0, float y0,
      float x1, float y1,
      float x2, float y2,
      Color color,
      float xmin, float ymin,
      float xmax, float ymax, float threshold);

  // rasterize a triangle
  void rasterize_triangle( float x0, float y0,
                           float x1, float y1,
                           float x2, float y2,
                           Color color );

  // rasterize an image
  void rasterize_image( float x0, float y0,
                        float x1, float y1,
                        Texture& tex );

  // resolve samples to render target
  void resolve( void );

  // Helpers //
  // HINT: you may want to have something similar //
  std::vector<unsigned char> sample_buffer; int w; int h;
  void fill_sample( int sx, int sy, const Color& c );
  void fill_pixel( int x, int y, const Color& c );

}; // class SoftwareRendererRef

} // namespace CMU462

#endif // CMU462_SOFTWARE_RENDERER_H
