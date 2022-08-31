/**
 * \file Animation2DAttachment.h
 * \brief Attachments are applied to slots and driven by the rigged skeleton
 * (or in the case of MeshAttachments, can also be driven by direct deformation).
 * Some attachments (Bitmap and Mesh) are renderable while others drive the
 * simulation or are used for runtime queries.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_ATTACHMENT_H
#define ANIMATION2D_ATTACHMENT_H

#include "FilePath.h"
#include "HashSet.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"
#include "StandardVertex2D.h"
#include "Vector.h"
#include "Vector2D.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { namespace Animation2D { class ReadWriteUtil; } }
namespace Seoul { namespace Reflection { struct SerializeContext; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

namespace Animation2D
{

/** This is the selected skin when no skin has been explicitly selected. */
static const HString kDefaultSkin("default");

enum class AttachmentType : UInt32
{
	kBitmap,
	kBoundingBox,
	kLinkedMesh,
	kMesh,
	kPath,
	kPoint,
	kClipping,
};

class Attachment SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Attachment);

	Attachment();
	virtual ~Attachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const = 0;

	// Direct to binary support.
	static Attachment* New(AttachmentType eType);
	virtual Bool Load(ReadWriteUtil& r) = 0;
	virtual Bool Save(ReadWriteUtil& r) const = 0;

	virtual AttachmentType GetType() const = 0;

protected:
	SEOUL_REFERENCE_COUNTED(Attachment);

private:
	SEOUL_DISABLE_COPY(Attachment);
}; // class Attachment

class BitmapAttachment SEOUL_SEALED : public Attachment
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(BitmapAttachment);

	BitmapAttachment();
	~BitmapAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kBitmap; }

	FilePath GetFilePath() const { return m_FilePath; }

	RGBA GetColor() const { return m_Color; }
	Float32 GetHeight() const { return m_fHeight; }
	// TODO: Compute a matrix transform on load and eliminate the need for these.
	Float32 GetPositionX() const { return m_fPositionX; }
	Float32 GetPositionY() const { return m_fPositionY; }
	Float32 GetRotationInDegrees() const { return m_fRotationInDegrees; }
	Float32 GetScaleX() const { return m_fScaleX; }
	Float32 GetScaleY() const { return m_fScaleY; }
	// /TODO:
	Float32 GetWidth() const { return m_fWidth; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BitmapAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(BitmapAttachment);

	RGBA m_Color;
	FilePath m_FilePath;
	Float32 m_fHeight;
	Float32 m_fPositionX;
	Float32 m_fPositionY;
	Float32 m_fRotationInDegrees;
	Float32 m_fScaleX;
	Float32 m_fScaleY;
	Float32 m_fWidth;

	Bool PostDeserialize();

	SEOUL_DISABLE_COPY(BitmapAttachment);
}; // class BitmapAttachment

class BoundingBoxAttachment SEOUL_SEALED : public Attachment
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(BoundingBoxAttachment);

	BoundingBoxAttachment();
	~BoundingBoxAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kBoundingBox; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BoundingBoxAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(BoundingBoxAttachment);
	SEOUL_DISABLE_COPY(BoundingBoxAttachment);
}; // class BoundingBoxAttachment

struct MeshAttachmentBoneLink SEOUL_SEALED
{
	MeshAttachmentBoneLink(UInt32 uIndex = 0u, Float32 fWeight = 0.0f)
		: m_uIndex(uIndex)
		, m_fWeight(fWeight)
	{
	}

	Bool operator==(const MeshAttachmentBoneLink& b) const
	{
		return
			(m_uIndex == b.m_uIndex) &&
			(m_fWeight == b.m_fWeight);
	}

	Bool operator!=(const MeshAttachmentBoneLink& b) const
	{
		return !(*this == b);
	}

	UInt32 m_uIndex;
	Float32 m_fWeight;
}; // struct MeshAttachmentBoneLink

/**
 * Utility structure used for computing texture resolutions
 * at runtime. Each entry describes a unique triangle
 * edge with the following additional data:
 * - 1.0 / (T1 - T0) (where T1 is UVs at 1 and T0 is UVs at 0)
 * - distance between the UVs squared.
 */
struct Edge SEOUL_SEALED
{
	static inline Edge Create(UInt16 u0 = 0, UInt16 u1 = 0)
	{
		Edge ret;
		ret.m_u0 = Min(u0, u1);
		ret.m_u1 = Max(u0, u1);
		ret.m_vAbsOneOverDiffT = Vector2D::Zero();
		ret.m_fSepSquared = 0.0f;
		return ret;
	}

	Vector2D m_vAbsOneOverDiffT{};
	Float32 m_fSepSquared{};
	union
	{
		struct
		{
			UInt16 m_u0;
			UInt16 m_u1;
		};
		UInt32 m_u{};
	};
}; // struct Edge
SEOUL_STATIC_ASSERT(sizeof(Edge) == sizeof(UInt32) + sizeof(Vector2D) + sizeof(Float32));

static inline Bool operator==(const Edge& a, const Edge& b) { return (a.m_u == b.m_u); }
static inline Bool operator!=(const Edge& a, const Edge& b) { return !(a.m_u == b.m_u); }
static inline Bool operator<(const Edge& a, const Edge& b) { return (a.m_fSepSquared > b.m_fSepSquared); }

static inline UInt32 GetHash(Edge edge) { return Seoul::GetHash(edge.m_u); }

} // namespace Animation2D
template <> struct CanMemCpy<Animation2D::MeshAttachmentBoneLink> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::MeshAttachmentBoneLink> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::Edge> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::Edge> { static const Bool Value = true; };

template <>
struct DefaultHashTableKeyTraits<Animation2D::Edge>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Animation2D::Edge GetNullKey()
	{
		return Animation2D::Edge::Create();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace Animation2D
{

class MeshAttachment SEOUL_SEALED : public Attachment
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(MeshAttachment);

	typedef Vector<Edge, MemoryBudgets::Animation2D> Edges;
	typedef HashSet<Edge, MemoryBudgets::Animation2D> EdgeSet;
	typedef Vector<UInt16, MemoryBudgets::Animation2D> Indices;
	typedef Vector<MeshAttachmentBoneLink, MemoryBudgets::Animation2D> Links;
	typedef Vector<Vector2D, MemoryBudgets::Animation2D> Vector2Ds;

	MeshAttachment();
	~MeshAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	void ComputeEdges();

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kMesh; }

	const Indices& GetBoneCounts() const { return m_vuBoneCounts; }
	RGBA GetColor() const { return m_Color; }
	const Edges& GetEdges() const { return m_vEdges; }
	FilePath GetFilePath() const { return m_FilePath; }
	Float32 GetHeight() const { return m_fHeight; }
	const Indices& GetIndices() const { return m_vuIndices; }
	const Links& GetLinks() const { return m_vLinks; }
	const Vector2Ds& GetTexCoords() const { return m_vTexCoords; }
	const Vector2Ds& GetVertices() const { return m_vVertices; }
	Float32 GetWidth() const { return m_fWidth; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(MeshAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(MeshAttachment);

	void InsertEdge(EdgeSet& set, UInt16 u0, UInt16 u1) const;

	RGBA m_Color;
	FilePath m_FilePath;
	Float32 m_fHeight;
	Int32 m_iHull;
	Edges m_vEdges;
	Indices m_vuIndices;
	Vector2Ds m_vTexCoords;
	Float32 m_fWidth;

	// Vertices have 2 possibilities:
	// - no skinning, in which case m_vVertices.GetSize() == m_vTexCoords.GetSize() and
	//   m_vLinks and m_vuBoneCounts will be empty.
	// - skinning, in which case:
	//   - m_vuBoneCounts.GetSize() == m_vTexCoords.GetSize()
	//   - each index in m_vBones defines a count, and there will be that many entries
	//     in m_vVertices and m_vLinks for each bone.
	Indices m_vuBoneCounts;
	Links m_vLinks;
	Vector2Ds m_vVertices;

	Bool CustomDeserializeTexCoords(
		Reflection::SerializeContext* pContext,
		DataStore const* pDataStore,
		const DataNode& value);
	Bool CustomDeserializeVertices(
		Reflection::SerializeContext* pContext,
		DataStore const* pDataStore,
		const DataNode& value);

	SEOUL_DISABLE_COPY(MeshAttachment);
}; // class MeshAttachment

class LinkedMeshAttachment SEOUL_SEALED : public Attachment
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(LinkedMeshAttachment);

	LinkedMeshAttachment();
	~LinkedMeshAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kLinkedMesh; }

	RGBA GetColor() const { return m_Color; }
	Bool GetDeform() const { return m_bDeform; }
	FilePath GetFilePath() const { return m_FilePath; }
	Float32 GetHeight() const { return m_fHeight; }
	const SharedPtr<MeshAttachment>& GetParent() const { return m_pParent; }
	HString GetParentId() const { return m_ParentId; }
	HString GetSkinId() const { return m_SkinId; }
	Float32 GetWidth() const { return m_fWidth; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(LinkedMeshAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(LinkedMeshAttachment);

	friend class DataDefinition;
	friend class ReadWriteUtil;
	RGBA m_Color;
	FilePath m_FilePath;
	Float32 m_fHeight;
	SharedPtr<MeshAttachment> m_pParent;
	HString m_ParentId;
	HString m_SkinId;
	Float32 m_fWidth;
	Bool m_bDeform;

	SEOUL_DISABLE_COPY(LinkedMeshAttachment);
}; // class LinkedMeshAttachment

class PathAttachment SEOUL_SEALED : public Attachment
{
public:
	typedef Vector<UInt16, MemoryBudgets::Animation2D> BoneCounts;
	typedef Vector<Float, MemoryBudgets::Animation2D> Lengths;
	typedef Vector<Float, MemoryBudgets::Animation2D> Vertices;

	SEOUL_REFLECTION_POLYMORPHIC(PathAttachment);

	PathAttachment();
	~PathAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kPath; }

	const BoneCounts& GetBoneCounts() const { return m_vBoneCounts; }
	Bool GetClosed() const { return m_bClosed; }
	Bool GetConstantSpeed() const { return m_bConstantSpeed; }
	HString GetId() const { return m_Id; }
	const Lengths& GetLengths() const { return m_vLengths; }
	HString GetSlot() const { return m_Slot; }
	UInt32 GetVertexCount() const { return m_uVertexCount; }
	const Vertices& GetVertices() const { return m_vVertices; }
	const Vertices& GetWeights() const { return m_vWeights; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PathAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(PathAttachment);

	friend class DataDefinition;
	BoneCounts m_vBoneCounts;
	Lengths m_vLengths;
	Vertices m_vVertices;
	Vertices m_vWeights;
	UInt32 m_uVertexCount;
	HString m_Id;
	HString m_Slot;
	Bool m_bClosed;
	Bool m_bConstantSpeed;

	Bool PostSerialize(Reflection::SerializeContext* pContext);

	SEOUL_DISABLE_COPY(PathAttachment);
}; // class PathAttachment

class PointAttachment SEOUL_SEALED : public Attachment
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(PointAttachment);

	PointAttachment();
	~PointAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kPoint; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PointAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(PointAttachment);

	Float32 m_fPositionX;
	Float32 m_fPositionY;
	Float32 m_fRotationInDegrees;

	SEOUL_DISABLE_COPY(PointAttachment);
}; // class PointAttachment

class ClippingAttachment SEOUL_SEALED : public Attachment
{
public:
	typedef Vector<UInt16, MemoryBudgets::Animation2D> BoneCounts;
	typedef Vector<Float, MemoryBudgets::Animation2D> Vertices;

	SEOUL_REFLECTION_POLYMORPHIC(ClippingAttachment);

	ClippingAttachment();
	~ClippingAttachment();

	// Comparison support.
	virtual Bool Equals(const Attachment& b) const SEOUL_OVERRIDE;

	// Direct to binary support.
	virtual Bool Load(ReadWriteUtil& r) SEOUL_OVERRIDE;
	virtual Bool Save(ReadWriteUtil& r) const SEOUL_OVERRIDE;

	virtual AttachmentType GetType() const SEOUL_OVERRIDE { return AttachmentType::kClipping; }

	const BoneCounts& GetBoneCounts() const { return m_vBoneCounts; }
	const Vertices& GetVertices() const { return m_vVertices; }

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(ClippingAttachment);
	SEOUL_REFLECTION_FRIENDSHIP(ClippingAttachment);

	friend class DataDefinition;
	BoneCounts m_vBoneCounts;
	Vertices m_vVertices;
	Vertices m_vWeights;
	UInt32 m_uVertexCount;

	Bool PostSerialize(Reflection::SerializeContext* pContext);

	SEOUL_DISABLE_COPY(ClippingAttachment);
}; // class ClippingAttachment

} // namespace Animation2D

static inline Bool operator==(const SharedPtr<Animation2D::Attachment>& pA, const SharedPtr<Animation2D::Attachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::Attachment>& pA, const SharedPtr<Animation2D::Attachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::BitmapAttachment>& pA, const SharedPtr<Animation2D::BitmapAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::BitmapAttachment>& pA, const SharedPtr<Animation2D::BitmapAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::BoundingBoxAttachment>& pA, const SharedPtr<Animation2D::BoundingBoxAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::BoundingBoxAttachment>& pA, const SharedPtr<Animation2D::BoundingBoxAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::LinkedMeshAttachment>& pA, const SharedPtr<Animation2D::LinkedMeshAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::LinkedMeshAttachment>& pA, const SharedPtr<Animation2D::LinkedMeshAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::MeshAttachment>& pA, const SharedPtr<Animation2D::MeshAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::MeshAttachment>& pA, const SharedPtr<Animation2D::MeshAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::PathAttachment>& pA, const SharedPtr<Animation2D::PathAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::PathAttachment>& pA, const SharedPtr<Animation2D::PathAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::PointAttachment>& pA, const SharedPtr<Animation2D::PointAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::PointAttachment>& pA, const SharedPtr<Animation2D::PointAttachment>& pB)
{
	return !(pA == pB);
}

static inline Bool operator==(const SharedPtr<Animation2D::ClippingAttachment>& pA, const SharedPtr<Animation2D::ClippingAttachment>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }
	if (pA->GetType() != pB->GetType()) { return false; }

	return pA->Equals(*pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::ClippingAttachment>& pA, const SharedPtr<Animation2D::ClippingAttachment>& pB)
{
	return !(pA == pB);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
