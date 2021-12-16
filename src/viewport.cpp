#include "viewport.h"
#include <iostream>
#include "CMU462.h"

namespace CMU462 {

	void ViewportImp::set_viewbox(float centerX, float centerY, float vspan) {


		// Task 5 (part 2): 
		// Set svg coordinate to normalized device coordinate transformation. Your input
		// arguments are defined as normalized SVG canvas coordinates.

		Matrix3x3 m = this->get_svg_2_norm();
		float width = 1 / m[0][0];
		float height = 1 / m[1][1];

		m[2][0] = -(centerX - width / 2) / width;
		m[2][1] = -(centerY - height / 2) / height;
		m[0][0] = .5/vspan ;
		m[1][1] = .5/vspan ;
		set_svg_2_norm(m);

		this->centerX = centerX;
		this->centerY = centerY;
		this->vspan = vspan;

	}

	void ViewportImp::update_viewbox(float dx, float dy, float scale) {

		this->centerX -= dx;
		this->centerY -= dy;
		this->vspan *= scale;
		set_viewbox(centerX, centerY, vspan);
	}

} // namespace CMU462
