#include "StdAfx.h"

#include "Cubism3Drawable.h"

namespace Cubism3 {
	Cubism3Drawable::Cubism3Drawable() {
		this->m_name = "";
		this->m_id = -1;
		this->m_constFlags = 0;
		this->m_dynFlags = 0;
		this->m_texId = -1;
		this->m_drawOrder = -1;
		this->m_renderOrder = -1;
		this->m_opacity = 1.0f;
		this->m_packedOpacity = 0;
		this->m_maskCount = -1;
		this->m_maskIndices = nullptr;
		this->m_transform = nullptr;
		this->m_uvTransform = nullptr;
		this->m_vertCount = -1;
		this->m_verts = nullptr;
		this->m_rawVerts = nullptr;
		this->m_rawUVs = nullptr;
		this->m_indicesCount = -1;
		this->m_indices = nullptr;
		this->m_visible = false;

		this->m_prevAnimOpacity = this->m_animOpacity = 1.0f;
	}

	void Cubism3Drawable::update(csmModel* model, bool transformUpdate, bool &renderOrderChanged, float opacity, bool opacityChanged) {
		this->m_dynFlags = csmGetDrawableDynamicFlags(model)[this->m_id];

		if (this->m_dynFlags & csmVisibilityDidChange) this->m_visible = this->m_dynFlags & csmIsVisible;

		if (this->m_dynFlags & csmDrawOrderDidChange) this->m_drawOrder = csmGetDrawableDrawOrders(model)[this->m_id];
		if (this->m_dynFlags & csmRenderOrderDidChange) {
			this->m_renderOrder = csmGetDrawableRenderOrders(model)[this->m_id];
			renderOrderChanged = true;
		}

		if (this->m_dynFlags & csmVertexPositionsDidChange) {
			//vertexes
			const int vertCount = csmGetDrawableVertexCounts(model)[this->m_id];

			//recreate buffer if needed
			if (vertCount != this->m_vertCount) {
				delete this->m_verts;
				this->m_vertCount = vertCount;
				this->m_verts = new SVF_P3F_C4B_T2F[this->m_vertCount];
			}

			this->m_rawVerts = csmGetDrawableVertexPositions(model)[this->m_id];
			this->m_rawUVs = csmGetDrawableVertexUvs(model)[this->m_id];

			//indicies
			const int icount = csmGetDrawableIndexCounts(model)[this->m_id];

			//recreate indices if needed
			if (this->m_indicesCount != icount) {
				delete this->m_indices;
				this->m_indicesCount = icount;
				this->m_indices = new uint16[this->m_indicesCount];
			}

			const unsigned short * in = csmGetDrawableIndices(model)[this->m_id];
			for (int i = 0; i < this->m_indicesCount; i++) this->m_indices[i] = in[i];

			transformUpdate = true; //make sure that we convert the data to lumberyard compatable data
		}

		if (this->m_dynFlags & csmOpacityDidChange || this->m_prevAnimOpacity != this->m_animOpacity || opacityChanged) {
			this->m_prevAnimOpacity = this->m_animOpacity;

			this->m_opacity = csmGetDrawableOpacities(model)[this->m_id] * this->m_animOpacity * opacity;
			this->m_packedOpacity = ColorF(1.0f, 1.0f, 1.0f, this->m_opacity).pack_argb8888();
			if (!transformUpdate) for (int i = 0; i < this->m_vertCount; i++) this->m_verts[i].color.dcolor = this->m_packedOpacity;
		}

		//update data only when transform has changed.
		if (transformUpdate) {
			for (int i = 0; i < this->m_vertCount; i++) {
				AZ::Vector3 vec(m_rawVerts[i].X, m_rawVerts[i].Y, 1.0f);
				vec = *this->m_transform * vec;

				AZ::Vector3 uvTrans = AZ::Vector3(this->m_rawUVs[i].X, this->m_rawUVs[i].Y, 0.0f) * *this->m_uvTransform;

				this->m_verts[i].xyz = Vec3(vec.GetX(), vec.GetY(), vec.GetZ());
				this->m_verts[i].st = Vec2(uvTrans.GetX(), uvTrans.GetY());
				this->m_verts[i].color.dcolor = this->m_packedOpacity;
			}
		}
	}
}