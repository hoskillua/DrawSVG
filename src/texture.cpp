#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

	inline void uint8_to_float(float dst[4], unsigned char* src) {
		uint8_t* src_uint8 = (uint8_t*)src;
		dst[0] = src_uint8[0] / 255.f;
		dst[1] = src_uint8[1] / 255.f;
		dst[2] = src_uint8[2] / 255.f;
		dst[3] = src_uint8[3] / 255.f;
	}

	inline void float_to_uint8(unsigned char* dst, float src[4]) {
		uint8_t* dst_uint8 = (uint8_t*)dst;
		dst_uint8[0] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[0])));
		dst_uint8[1] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[1])));
		dst_uint8[2] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[2])));
		dst_uint8[3] = (uint8_t)(255.f * max(0.0f, min(1.0f, src[3])));
	}

	void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

		// NOTE: 
		// This starter code allocates the mip levels and generates a level 
		// map by filling each level with placeholder data in the form of a 
		// color that differs from its neighbours'. You should instead fill
		// with the correct data!

		// Task 7: Implement this

		// check start level
		if (startLevel >= tex.mipmap.size()) {
			std::cerr << "Invalid start level";
		}

		// allocate sublevels
		int baseWidth = tex.mipmap[startLevel].width;
		int baseHeight = tex.mipmap[startLevel].height;
		int numSubLevels = (int)(log2f((float)max(baseWidth, baseHeight)));

		numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
		tex.mipmap.resize(startLevel + numSubLevels + 1);

		int width = baseWidth;
		int height = baseHeight;
		for (int i = 1; i <= numSubLevels; i++) {

			MipLevel& level = tex.mipmap[startLevel + i];

			// handle odd size texture by rounding down
			width = max(1, width / 2); assert(width > 0);
			height = max(1, height / 2); assert(height > 0);

			level.width = width;
			level.height = height;
			level.texels = vector<unsigned char>(4 * width * height);

		}

		// fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
		for (size_t i = 1; i < tex.mipmap.size(); ++i) {


			MipLevel& mipCurr = tex.mipmap[i];
			MipLevel& mipPrev = tex.mipmap[i - 1];

			int j = 0;
			int w = 0;
			for (size_t i = 0; i < 4 * mipCurr.width * mipCurr.height; i += 4) {
				j = i * 2;
				w = mipCurr.width * 2;
				int x = (j / 4) % w;
				int y = (j / 4) / w;
				unsigned char* src00 = &mipPrev.texels[(x + y * 2* w) * 4];
				unsigned char* src01 = &mipPrev.texels[(x + (y + 1) * 2 * w) * 4];
				unsigned char* src10 = &mipPrev.texels[(x + 1 + y * 2 * w) * 4];
				unsigned char* src11 = &mipPrev.texels[(x + 1 + (y + 1) * 2 * w) * 4];
				//cout << i << " " << (x + y * 2 * w) * 4 << endl;
				float src_avg[4] = {
				 (src00[0] * (0.25f / 255) + src10[0] * (0.25f/255)) + (src01[0] * (0.25f/255) + src11[0] * (0.25f/255)),
				 (src00[1] * (0.25f / 255) + src10[1] * (0.25f/255)) + (src01[1] * (0.25f/255) + src11[1] * (0.25f/255)),
				 (src00[2] * (0.25f / 255) + src10[2] * (0.25f/255)) + (src01[2] * (0.25f/255) + src11[2] * (0.25f/255)),
				 (src00[3] * (0.25f / 255) + src10[3] * (0.25f/255)) + (src01[3] * (0.25f/255) + src11[3] * (0.25f/255))
				};
				float_to_uint8(&mipCurr.texels[i], src_avg);
			}
		}

	}

	Color Sampler2DImp::sample_nearest(Texture& tex,
		float u, float v,
		int level) {
		if (level < tex.mipmap.size() && level >= 0)
		{
			// Task 6: Implement nearest neighbour interpolation
			MipLevel& mip = tex.mipmap[level];
			float dst[4];
			int x = round(u * tex.width);
			int y = round(v * tex.height);
			if ((x + y * tex.width) * 4 >= 0 && (x + y * tex.width) * 4 < mip.texels.size())
			{
				unsigned char* src = &mip.texels[(x + y * tex.width) * 4];
				uint8_to_float(dst, src);
				return Color(dst[0], dst[1], dst[2], dst[3]);
			}
		}
		// return magenta for invalid level
		return Color(1, 0, 1, 1);

	}

	Color Sampler2DImp::sample_bilinear(Texture& tex,
		float u, float v,
		int level) {

		if (level < tex.mipmap.size() && level >= 0)
		{
			MipLevel& mip = tex.mipmap[level];
			int x0 = floor(u * mip.width);
			int y0 = floor(v * mip.height);
			int x1 = floor((u * mip.width) + 1);
			int y1 = floor((v * mip.height) + 1);
			float t = u * mip.width - x0;
			float s = v * mip.height - y0;
			float dst00[4];
			float dst01[4];
			float dst10[4];
			float dst11[4];
			int l = mip.texels.size();
			unsigned char* src00 = &mip.texels[(x0 + y0 * mip.width) * 4 % l];
			unsigned char* src01 = &mip.texels[(x0 + y1 * mip.width) * 4 % l];
			unsigned char* src10 = &mip.texels[(x1 + y0 * mip.width) * 4 % l];
			unsigned char* src11 = &mip.texels[(x1 + y1 * mip.width) * 4 % l];
			uint8_to_float(dst00, src00);
			uint8_to_float(dst01, src01);
			uint8_to_float(dst10, src10);
			uint8_to_float(dst11, src11);
			float dst_avg[4] = {
				(1 - s) * (dst00[0] * (1 - t) + dst10[0] * (t)) + s * (dst01[0] * (1 - t) + dst11[0] * (t)),
				(1 - s) * (dst00[1] * (1 - t) + dst10[1] * (t)) + s * (dst01[1] * (1 - t) + dst11[1] * (t)),
				(1 - s) * (dst00[2] * (1 - t) + dst10[2] * (t)) + s * (dst01[2] * (1 - t) + dst11[2] * (t)),
				(1 - s) * (dst00[3] * (1 - t) + dst10[3] * (t)) + s * (dst01[3] * (1 - t) + dst11[3] * (t))
			};
			return Color(dst_avg[0], dst_avg[1], dst_avg[2], dst_avg[3]);
		}
		// return magenta for invalid level
		return Color(1, 0, 1, 1);

	}

	Color Sampler2DImp::sample_trilinear(Texture& tex,
		float u, float v,
		float u_scale, float v_scale) {

		float r = log(u_scale) / log(2);
		if (r < tex.mipmap.size() && r >= 0)
		{
			int rfloor = floor(r);
			return (1 - (r - rfloor)) * sample_bilinear(tex, u, v, rfloor) + (r - rfloor) * sample_bilinear(tex, u, v, rfloor + 1);
		}
		// Task 7: Implement trilinear filtering

		// return magenta for invalid level
		return Color(1, 0, 1, 1);

	}

} // namespace CMU462
