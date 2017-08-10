#pragma once

#include <Cry_Color.h>
#include <VertexFormats.h>
#include <AzCore/Math/Matrix4x4.h>

#include "Live2DCubismCore.h"

namespace Cubism3 {
	class Cubism3Drawable {
	public:
		Cubism3Drawable();

	public:
		AZStd::string m_name; //csmGetDrawableIds
		int m_id;
		csmFlags m_constFlags; //csmGetDrawableConstantFlags
		csmFlags m_dynFlags; //csmGetDrawableDynamicFlags //constant update?
		int m_texId; //csmGetDrawableTextureIndices //used when using model3.json
		int m_drawOrder; //csmGetDrawableDrawOrders
		int m_renderOrder; //csmGetDrawableRenderOrders
		float m_opacity; //csmGetDrawableOpacities //color (alpha) //update as needed?
		uint32 m_packedOpacity;

		int m_maskCount; //csmGetDrawableMaskCounts
		uint16 * m_maskIndices; //csmGetDrawableMasks

		AZ::Matrix4x4 *m_transform, *m_uvTransform;

		int m_vertCount;
		SVF_P3F_C4B_T2F * m_verts; //csmGetDrawableVertexPositions, csmGetDrawableVertexUvs //update only when needed to?

		const csmVector2* m_rawVerts;
		const csmVector2* m_rawUVs;

		int m_indicesCount; //csmGetDrawableIndexCounts
		uint16 * m_indices; //csmGetDrawableIndices

		bool m_visible;

	public:
		float m_prevAnimOpacity, m_animOpacity;

	public:
		void update(csmModel* model, bool transformUpdate, bool &renderOrderChanged, float opacity, bool opacityChanged);
	};
}