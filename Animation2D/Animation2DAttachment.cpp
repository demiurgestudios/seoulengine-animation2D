/**
 * \file Animation2DAttachment.cpp
 * \brief Attachments are applied to slots and driven by the rigged skeleton
 * (or in the case of MeshAttachments, can also be driven by direct deformation).
 * Some attachments (Bitmap and Mesh) are renderable while others drive the
 * simulation or are used for runtime queries.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "Animation2DAttachment.h"
#include "Animation2DReadWriteUtil.h"
#include "HashSet.h"
#include "Matrix2x3.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, HashTable<HString, HashTable<HString, SharedPtr<Animation2D::Attachment>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, HashTable<HString, SharedPtr<Animation2D::Attachment>, 2, DefaultHashTableKeyTraits<HString>>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, SharedPtr<Animation2D::Attachment>, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(SharedPtr<Animation2D::Attachment>)
SEOUL_BEGIN_ENUM(Animation2D::AttachmentType)
	SEOUL_ENUM_N("Bitmap", Animation2D::AttachmentType::kBitmap)
	SEOUL_ENUM_N("LinkedMesh", Animation2D::AttachmentType::kLinkedMesh)
	SEOUL_ENUM_N("Mesh", Animation2D::AttachmentType::kMesh)
	SEOUL_ENUM_N("Path", Animation2D::AttachmentType::kPath)
	SEOUL_ENUM_N("Point", Animation2D::AttachmentType::kPoint)
	SEOUL_ENUM_N("Clipping", Animation2D::AttachmentType::kClipping)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Animation2D::Attachment, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(PolymorphicKey, "type", "region")
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::BitmapAttachment, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("region")

	SEOUL_PROPERTY_N("color", m_Color)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
	SEOUL_PROPERTY_N("height", m_fHeight)
	SEOUL_PROPERTY_N("x", m_fPositionX)
	SEOUL_PROPERTY_N("y", m_fPositionY)
	SEOUL_PROPERTY_N("rotation", m_fRotationInDegrees)
	SEOUL_PROPERTY_N("scaleX", m_fScaleX)
	SEOUL_PROPERTY_N("scaleY", m_fScaleY)
	SEOUL_PROPERTY_N("width", m_fWidth)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::BoundingBoxAttachment, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("boundingbox")
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::LinkedMeshAttachment, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("linkedmesh")

	SEOUL_PROPERTY_N("color", m_Color)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
	SEOUL_PROPERTY_N("deform", m_bDeform)
	SEOUL_PROPERTY_N("height", m_fHeight)
	SEOUL_PROPERTY_N("parent", m_ParentId)
	SEOUL_PROPERTY_N("skin", m_SkinId)
	SEOUL_PROPERTY_N("width", m_fWidth)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::MeshAttachment, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("mesh")

	SEOUL_PROPERTY_N("color", m_Color)
	SEOUL_PROPERTY_N("FilePath", m_FilePath)
	SEOUL_PROPERTY_N("hull", m_iHull)
	SEOUL_PROPERTY_N("triangles", m_vuIndices)
	SEOUL_PROPERTY_N("uvs", m_vTexCoords)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "CustomDeserializeTexCoords")
	SEOUL_PROPERTY_N("vertices", m_vVertices)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "CustomDeserializeVertices")

	SEOUL_METHOD(CustomDeserializeTexCoords)
	SEOUL_METHOD(CustomDeserializeVertices)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::PathAttachment, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(PostSerializeType, "PostSerialize")
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("path")
	SEOUL_PROPERTY_N("closed", m_bClosed)
	SEOUL_PROPERTY_N("constantSpeed", m_bConstantSpeed)
	SEOUL_PROPERTY_N("lengths", m_vLengths)
	SEOUL_PROPERTY_N("vertexCount", m_uVertexCount)
	SEOUL_PROPERTY_N("vertices", m_vVertices)

	SEOUL_METHOD(PostSerialize)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::PointAttachment, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("point")

	SEOUL_PROPERTY_N("x", m_fPositionX)
	SEOUL_PROPERTY_N("y", m_fPositionY)
	SEOUL_PROPERTY_N("rotation", m_fRotationInDegrees)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::ClippingAttachment, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(PostSerializeType, "PostSerialize")
	SEOUL_PARENT(Animation2D::Attachment)
	SEOUL_TYPE_ALIAS("clipping")
	SEOUL_PROPERTY_N("vertexCount", m_uVertexCount)
	SEOUL_PROPERTY_N("vertices", m_vVertices)

	SEOUL_METHOD(PostSerialize)
SEOUL_END_TYPE()

namespace Animation2D
{

Attachment::Attachment()
{
}

Attachment::~Attachment()
{
}

Attachment* Attachment::New(AttachmentType eType)
{
	switch (eType)
	{
	case AttachmentType::kBitmap: return SEOUL_NEW(MemoryBudgets::Animation2D) BitmapAttachment;
	case AttachmentType::kBoundingBox: return SEOUL_NEW(MemoryBudgets::Animation2D) BoundingBoxAttachment;
	case AttachmentType::kLinkedMesh: return SEOUL_NEW(MemoryBudgets::Animation2D) LinkedMeshAttachment;
	case AttachmentType::kMesh: return SEOUL_NEW(MemoryBudgets::Animation2D) MeshAttachment;
	case AttachmentType::kPath: return SEOUL_NEW(MemoryBudgets::Animation2D) PathAttachment;
	case AttachmentType::kPoint: return SEOUL_NEW(MemoryBudgets::Animation2D) PointAttachment;
	case AttachmentType::kClipping: return SEOUL_NEW(MemoryBudgets::Animation2D) ClippingAttachment;
	default:
		return nullptr;
	};
}

BitmapAttachment::BitmapAttachment()
	: m_Color(RGBA::White())
	, m_FilePath()
	, m_fHeight(32.0f)
	, m_fPositionX(0.0f)
	, m_fPositionY(0.0f)
	, m_fRotationInDegrees(0.0f)
	, m_fScaleX(1.0f)
	, m_fScaleY(1.0f)
	, m_fWidth(32.0f)
{
}

BitmapAttachment::~BitmapAttachment()
{
}

Bool BitmapAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kBitmap);
	auto const& b = *((BitmapAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_Color == b.m_Color);
	bReturn = bReturn && (m_FilePath == b.m_FilePath);
	bReturn = bReturn && (m_fHeight == b.m_fHeight);
	bReturn = bReturn && (m_fPositionX == b.m_fPositionX);
	bReturn = bReturn && (m_fPositionY == b.m_fPositionY);
	bReturn = bReturn && (m_fRotationInDegrees == b.m_fRotationInDegrees);
	bReturn = bReturn && (m_fScaleX == b.m_fScaleX);
	bReturn = bReturn && (m_fScaleY == b.m_fScaleY);
	bReturn = bReturn && (m_fWidth == b.m_fWidth);
	return bReturn;
}

Bool BitmapAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_Color);
	bReturn = bReturn && r.Read(m_FilePath);
	bReturn = bReturn && r.Read(m_fHeight);
	bReturn = bReturn && r.Read(m_fPositionX);
	bReturn = bReturn && r.Read(m_fPositionY);
	bReturn = bReturn && r.Read(m_fRotationInDegrees);
	bReturn = bReturn && r.Read(m_fScaleX);
	bReturn = bReturn && r.Read(m_fScaleY);
	bReturn = bReturn && r.Read(m_fWidth);
	return bReturn;
}

Bool BitmapAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_Color);
	bReturn = bReturn && r.Write(m_FilePath);
	bReturn = bReturn && r.Write(m_fHeight);
	bReturn = bReturn && r.Write(m_fPositionX);
	bReturn = bReturn && r.Write(m_fPositionY);
	bReturn = bReturn && r.Write(m_fRotationInDegrees);
	bReturn = bReturn && r.Write(m_fScaleX);
	bReturn = bReturn && r.Write(m_fScaleY);
	bReturn = bReturn && r.Write(m_fWidth);
	return bReturn;
}

BoundingBoxAttachment::BoundingBoxAttachment()
{
}

BoundingBoxAttachment::~BoundingBoxAttachment()
{
}

Bool BoundingBoxAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kBitmap);

	return true;
}

Bool BoundingBoxAttachment::Load(ReadWriteUtil& r)
{
	return true;
}

Bool BoundingBoxAttachment::Save(ReadWriteUtil& r) const
{
	return true;
}

LinkedMeshAttachment::LinkedMeshAttachment()
	: m_Color(RGBA::White())
	, m_FilePath()
	, m_fHeight(32.0f)
	, m_pParent()
	, m_ParentId()
	, m_SkinId()
	, m_fWidth(32.0f)
	, m_bDeform(true)
{
}

LinkedMeshAttachment::~LinkedMeshAttachment()
{
}

Bool LinkedMeshAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kLinkedMesh);
	auto const& b = *((LinkedMeshAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_Color == b.m_Color);
	bReturn = bReturn && (m_FilePath == b.m_FilePath);
	bReturn = bReturn && (m_fHeight == b.m_fHeight);
	bReturn = bReturn && (m_pParent == b.m_pParent);
	bReturn = bReturn && (m_ParentId == b.m_ParentId);
	bReturn = bReturn && (m_SkinId == b.m_SkinId);
	bReturn = bReturn && (m_fWidth == b.m_fWidth);
	bReturn = bReturn && (m_bDeform == b.m_bDeform);
	return bReturn;
}

Bool LinkedMeshAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_Color);
	bReturn = bReturn && r.Read(m_FilePath);
	bReturn = bReturn && r.Read(m_fHeight);
	bReturn = bReturn && r.Read(m_ParentId);
	bReturn = bReturn && r.Read(m_SkinId);
	bReturn = bReturn && r.Read(m_fWidth);
	bReturn = bReturn && r.Read(m_bDeform);
	return bReturn;
}

Bool LinkedMeshAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_Color);
	bReturn = bReturn && r.Write(m_FilePath);
	bReturn = bReturn && r.Write(m_fHeight);
	bReturn = bReturn && r.Write(m_ParentId);
	bReturn = bReturn && r.Write(m_SkinId);
	bReturn = bReturn && r.Write(m_fWidth);
	bReturn = bReturn && r.Write(m_bDeform);
	return bReturn;
}

MeshAttachment::MeshAttachment()
	: m_Color(RGBA::White())
	, m_FilePath()
	, m_fHeight(32.0f)
	, m_iHull(0)
	, m_vuIndices()
	, m_vTexCoords()
	, m_fWidth(32.0f)
	, m_vuBoneCounts()
	, m_vLinks()
	, m_vVertices()
{
}

MeshAttachment::~MeshAttachment()
{
}

Bool MeshAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kMesh);
	auto const& b = *((MeshAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_Color == b.m_Color);
	bReturn = bReturn && (m_FilePath == b.m_FilePath);
	bReturn = bReturn && (m_fHeight == b.m_fHeight);
	bReturn = bReturn && (m_iHull == b.m_iHull);
	bReturn = bReturn && (m_vEdges == b.m_vEdges);
	bReturn = bReturn && (m_vuIndices == b.m_vuIndices);
	bReturn = bReturn && (m_vTexCoords == b.m_vTexCoords);
	bReturn = bReturn && (m_fWidth == b.m_fWidth);
	bReturn = bReturn && (m_vuBoneCounts == b.m_vuBoneCounts);
	bReturn = bReturn && (m_vLinks == b.m_vLinks);
	bReturn = bReturn && (m_vVertices == b.m_vVertices);
	return bReturn;
}

Bool MeshAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_Color);
	bReturn = bReturn && r.Read(m_FilePath);
	bReturn = bReturn && r.Read(m_fHeight);
	bReturn = bReturn && r.Read(m_iHull);
	bReturn = bReturn && r.Read(m_vEdges);
	bReturn = bReturn && r.Read(m_vuIndices);
	bReturn = bReturn && r.Read(m_vTexCoords);
	bReturn = bReturn && r.Read(m_fWidth);
	bReturn = bReturn && r.Read(m_vuBoneCounts);
	bReturn = bReturn && r.Read(m_vLinks);
	bReturn = bReturn && r.Read(m_vVertices);
	return bReturn;
}

Bool MeshAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_Color);
	bReturn = bReturn && r.Write(m_FilePath);
	bReturn = bReturn && r.Write(m_fHeight);
	bReturn = bReturn && r.Write(m_iHull);
	bReturn = bReturn && r.Write(m_vEdges);
	bReturn = bReturn && r.Write(m_vuIndices);
	bReturn = bReturn && r.Write(m_vTexCoords);
	bReturn = bReturn && r.Write(m_fWidth);
	bReturn = bReturn && r.Write(m_vuBoneCounts);
	bReturn = bReturn && r.Write(m_vLinks);
	bReturn = bReturn && r.Write(m_vVertices);
	return bReturn;
}

void MeshAttachment::ComputeEdges()
{
	// TODO: Move this into the cooker.

	// TODO: Configure - three edges is 3 triangles worth
	// of unique edges.
	static const UInt32 kuMaxEdges = 9u;

	{
		// Build the unique edge set.
		EdgeSet edges; // TODO: Eliminate heap allocation.
		UInt32 const uIndices = m_vuIndices.GetSize();
		for (UInt32 i = 0u; i < uIndices; i += 3u)
		{
			auto const u0 = m_vuIndices[i + 0u];
			auto const u1 = m_vuIndices[i + 1u];
			auto const u2 = m_vuIndices[i + 2u];
			InsertEdge(edges, u0, u1);
			InsertEdge(edges, u1, u2);
			InsertEdge(edges, u2, u0);
		}

		// Now fill out edges.
		m_vEdges.Reserve(edges.GetSize());
		for (auto const& edge : edges)
		{
			m_vEdges.PushBack(edge);
		}
	}

	// Sort.
	QuickSort(m_vEdges.Begin(), m_vEdges.End());

	// Restrict.
	m_vEdges.Resize(Min(m_vEdges.GetSize(), kuMaxEdges));
}

/**
 * Insert an edge - if successful, compute terms that
 * will later be used for texture resolution computation.
 */
void MeshAttachment::InsertEdge(EdgeSet& edges, UInt16 u0, UInt16 u1) const
{
	auto const e = edges.Insert(Edge::Create(u0, u1));
	if (e.Second)
	{
		auto const& t0 = m_vTexCoords[u0];
		auto const& t1 = m_vTexCoords[u1];
		auto const vDiff = (t1 - t0);

		// Compute distance squared.
		e.First->m_fSepSquared = vDiff.LengthSquared();

		// If separation is totally zero, remove this element.
		if (IsZero(e.First->m_fSepSquared))
		{
			auto const edge = *e.First;
			edges.Erase(edge);
			return;
		}

		// Now compute the inverse diff along each axis.
		e.First->m_vAbsOneOverDiffT.X = IsZero(vDiff.X) ? 0.0f : Abs(1.0f / vDiff.X);
		e.First->m_vAbsOneOverDiffT.Y = IsZero(vDiff.Y) ? 0.0f : Abs(1.0f / vDiff.Y);
	}
}

static Bool ToVector2DArray(
	Reflection::SerializeContext& context,
	const DataStore& dataStore,
	const DataNode& arr,
	MeshAttachment::Vector2Ds& rv)
{
	UInt32 uArrayCount = 0u;
	SEOUL_VERIFY(dataStore.GetArrayCount(arr, uArrayCount));
	if (0u != (uArrayCount % 2u))
	{
		context.HandleError(Reflection::SerializeError::kFailedSettingValueToArray);
		return false;
	}

	rv.Clear();
	rv.Reserve(uArrayCount / 2u);
	DataNode node;
	for (UInt32 i = 0u; i < uArrayCount; i += 2u)
	{
		SEOUL_VERIFY(dataStore.GetValueFromArray(arr, i+0u, node));
		Float fU;
		if (!dataStore.AsFloat32(node, fU))
		{
			context.HandleError(Reflection::SerializeError::kFailedSettingValueToArray);
			return false;
		}

		SEOUL_VERIFY(dataStore.GetValueFromArray(arr, i+1u, node));
		Float fV;
		if (!dataStore.AsFloat32(node, fV))
		{
			context.HandleError(Reflection::SerializeError::kFailedSettingValueToArray);
			return false;
		}

		rv.PushBack(Vector2D(fU, fV));
	}

	return true;
}

Bool MeshAttachment::CustomDeserializeTexCoords(
	Reflection::SerializeContext* pContext,
	DataStore const* pDataStore,
	const DataNode& value)
{
	if (!value.IsArray())
	{
		pContext->HandleError(Reflection::SerializeError::kDataNodeIsNotArray);
		return false;
	}

	return ToVector2DArray(
		*pContext,
		*pDataStore,
		value,
		m_vTexCoords);
}

Bool MeshAttachment::CustomDeserializeVertices(
	Reflection::SerializeContext* pContext,
	DataStore const* pDataStore,
	const DataNode& value)
{
	if (!value.IsArray())
	{
		pContext->HandleError(Reflection::SerializeError::kDataNodeIsNotArray);
		return false;
	}

	DataStoreArrayUtil util(*pDataStore, value);
	UInt32 const uArrayCount = util.GetCount();
	UInt32 const uTexCoords = m_vTexCoords.GetSize();

	Bool bReturn = false;

	// If the input array length is equal to
	// the number of UVs we've already read (* 2, since we already packed them
	// into Vector2Ds), then the input is just an array of vertex positions.
	if (2u * uTexCoords == uArrayCount)
	{
		m_vuBoneCounts.Clear();
		m_vLinks.Clear();
		bReturn = ToVector2DArray(
			*pContext,
			*pDataStore,
			value,
			m_vVertices);
	}
	// Otherwise, the array is layed out as follows:
	// - first is a bone count,
	// - followed by <bone-count> entries, where each entry has 4 components:
	//   - bone index
	//   - position x
	//   - position y
	//    - weight.
	else
	{
		// Clear all three, then reserve. Bone is exact, vertices/links
		// are a best case (there will be at least one link and one vertex
		// for each bone entry).
		m_vuBoneCounts.Clear();
		m_vLinks.Clear();
		m_vVertices.Clear();

		m_vuBoneCounts.Reserve(uTexCoords);
		m_vLinks.Reserve(uTexCoords);
		m_vVertices.Reserve(uTexCoords);

		for (UInt32 i = 0u; i < uArrayCount; )
		{
			UInt32 uBoneCount = 0u;
			if (!util.GetValue(i++, uBoneCount))
			{
				pContext->HandleError(Reflection::SerializeError::kFailedSettingValueToArray);
				return false;
			}

			UInt32 const uEnd = (i + (4u * uBoneCount));
			for (; i < uEnd; )
			{
				UInt32 uBoneIndex = 0u;
				Float32 fX = 0.0f;
				Float32 fY = 0.0f;
				Float32 fWeight = 0.0f;
				if (!util.GetValue(i++, uBoneIndex) ||
					!util.GetValue(i++, fX) ||
					!util.GetValue(i++, fY) ||
					!util.GetValue(i++, fWeight))
				{
					pContext->HandleError(Reflection::SerializeError::kFailedSettingValueToArray);
					return false;
				}

				m_vLinks.PushBack(MeshAttachmentBoneLink(uBoneIndex, fWeight));
				m_vVertices.PushBack(Vector2D(fX, fY));
			}

			// Add the bone lookup.
			m_vuBoneCounts.PushBack((UInt16)uBoneCount);
		}

		bReturn = true;
	}

	// Compute width and height.
	if (bReturn && !m_vVertices.IsEmpty())
	{
		Vector2D vMin(Vector2D(FloatMax, FloatMax));
		Vector2D vMax(Vector2D(-FloatMax, -FloatMax));

		for (auto i = m_vVertices.Begin(); m_vVertices.End() != i; ++i)
		{
			vMin = Vector2D::Min(vMin, *i);
			vMax = Vector2D::Max(vMax, *i);
		}

		m_fHeight = Abs(vMax.Y - vMin.Y);
		m_fWidth = Abs(vMax.X - vMin.X);
	}
	else
	{
		m_fHeight = 32.0f;
		m_fWidth = 32.0f;
	}

	return bReturn;
}

PathAttachment::PathAttachment()
	: m_vBoneCounts()
	, m_vLengths()
	, m_vVertices()
	, m_vWeights()
	, m_uVertexCount(0u)
	, m_bClosed(false)
	, m_bConstantSpeed(true)
{
}

PathAttachment::~PathAttachment()
{
}

Bool PathAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kPath);
	auto const& b = *((PathAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_vBoneCounts == b.m_vBoneCounts);
	bReturn = bReturn && (m_vLengths == b.m_vLengths);
	bReturn = bReturn && (m_vVertices == b.m_vVertices);
	bReturn = bReturn && (m_vWeights == b.m_vWeights);
	bReturn = bReturn && (m_uVertexCount == b.m_uVertexCount);
	bReturn = bReturn && (m_Id == b.m_Id);
	bReturn = bReturn && (m_Slot == b.m_Slot);
	bReturn = bReturn && (m_bClosed == b.m_bClosed);
	bReturn = bReturn && (m_bConstantSpeed == b.m_bConstantSpeed);
	return bReturn;
}

Bool PathAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_vBoneCounts);
	bReturn = bReturn && r.Read(m_vLengths);
	bReturn = bReturn && r.Read(m_vVertices);
	bReturn = bReturn && r.Read(m_vWeights);
	bReturn = bReturn && r.Read(m_uVertexCount);
	bReturn = bReturn && r.Read(m_Id);
	bReturn = bReturn && r.Read(m_Slot);
	bReturn = bReturn && r.Read(m_bClosed);
	bReturn = bReturn && r.Read(m_bConstantSpeed);
	return bReturn;
}

Bool PathAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_vBoneCounts);
	bReturn = bReturn && r.Write(m_vLengths);
	bReturn = bReturn && r.Write(m_vVertices);
	bReturn = bReturn && r.Write(m_vWeights);
	bReturn = bReturn && r.Write(m_uVertexCount);
	bReturn = bReturn && r.Write(m_Id);
	bReturn = bReturn && r.Write(m_Slot);
	bReturn = bReturn && r.Write(m_bClosed);
	bReturn = bReturn && r.Write(m_bConstantSpeed);
	return bReturn;
}

Bool PathAttachment::PostSerialize(Reflection::SerializeContext* pContext)
{
	// Not sure why we need to do this, but matching
	// the spine API (not sure why this isn't just
	// 2x in the input data).
	m_uVertexCount *= 2u;

	// Early out if no weighting is present
	// for the vertices.
	if (m_uVertexCount == m_vVertices.GetSize())
	{
		m_vBoneCounts.Clear();
		return true;
	}

	// Otherwise, break vertices into bones and weights,
	// and update appropriately.
	Vertices vVertices;
	vVertices.Reserve(m_uVertexCount * 2u * 3u);
	Vertices vWeights;
	vWeights.Reserve(m_uVertexCount * 3u);
	BoneCounts vBoneCounts;
	vBoneCounts.Reserve(m_uVertexCount * 3u);

	UInt32 const uVertices = m_vVertices.GetSize();
	for (auto i = 0u; i < uVertices; )
	{
		auto const f = m_vVertices[i++];
		UInt16 const uBoneCount = (UInt16)((Int32)f);

		vBoneCounts.PushBack(uBoneCount);
		for (UInt32 j = i + (UInt32)uBoneCount * 4u; i < j; i += 4u)
		{
			vBoneCounts.PushBack((UInt16)((Int32)m_vVertices[i]));
			vVertices.PushBack(m_vVertices[i + 1u]);
			vVertices.PushBack(m_vVertices[i + 2u]);
			vWeights.PushBack(m_vVertices[i + 3u]);
		}
	}

	// Replace and complete.
	m_vBoneCounts.Swap(vBoneCounts);
	m_vVertices.Swap(vVertices);
	m_vWeights.Swap(vWeights);
	return true;
}

PointAttachment::PointAttachment()
	: m_fPositionX(0.0f)
	, m_fPositionY(0.0f)
	, m_fRotationInDegrees(0.0f)
{
}

PointAttachment::~PointAttachment()
{
}

Bool PointAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kPoint);
	auto const& b = *((PointAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_fPositionX == b.m_fPositionX);
	bReturn = bReturn && (m_fPositionY == b.m_fPositionY);
	bReturn = bReturn && (m_fRotationInDegrees == b.m_fRotationInDegrees);
	return bReturn;
}

Bool PointAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_fPositionX);
	bReturn = bReturn && r.Read(m_fPositionY);
	bReturn = bReturn && r.Read(m_fRotationInDegrees);
	return bReturn;
}

Bool PointAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_fPositionX);
	bReturn = bReturn && r.Write(m_fPositionY);
	bReturn = bReturn && r.Write(m_fRotationInDegrees);
	return bReturn;
}

ClippingAttachment::ClippingAttachment()
	: m_vBoneCounts()
	, m_vVertices()
	, m_vWeights()
	, m_uVertexCount(0u)
{
}

ClippingAttachment::~ClippingAttachment()
{
}

Bool ClippingAttachment::Equals(const Attachment& inB) const
{
	SEOUL_ASSERT(inB.GetType() == AttachmentType::kClipping);
	auto const& b = *((ClippingAttachment const*)&inB);

	Bool bReturn = true;
	bReturn = bReturn && (m_vBoneCounts == b.m_vBoneCounts);
	bReturn = bReturn && (m_vVertices == b.m_vVertices);
	bReturn = bReturn && (m_vWeights == b.m_vWeights);
	bReturn = bReturn && (m_uVertexCount == b.m_uVertexCount);
	return bReturn;
}

Bool ClippingAttachment::Load(ReadWriteUtil& r)
{
	Bool bReturn = true;
	bReturn = bReturn && r.Read(m_vBoneCounts);
	bReturn = bReturn && r.Read(m_vVertices);
	bReturn = bReturn && r.Read(m_vWeights);
	bReturn = bReturn && r.Read(m_uVertexCount);
	return bReturn;
}

Bool ClippingAttachment::Save(ReadWriteUtil& r) const
{
	Bool bReturn = true;
	bReturn = bReturn && r.Write(m_vBoneCounts);
	bReturn = bReturn && r.Write(m_vVertices);
	bReturn = bReturn && r.Write(m_vWeights);
	bReturn = bReturn && r.Write(m_uVertexCount);
	return bReturn;
}

Bool ClippingAttachment::PostSerialize(Reflection::SerializeContext* pContext)
{
	// Not sure why we need to do this, but matching
	// the spine API (not sure why this isn't just
	// 2x in the input data).
	m_uVertexCount *= 2u;

	// Early out if no weighting is present
	// for the vertices.
	if (m_uVertexCount == m_vVertices.GetSize())
	{
		m_vBoneCounts.Clear();
		return true;
	}

	// Otherwise, break vertices into bones and weights,
	// and update appropriately.
	Vertices vVertices;
	vVertices.Reserve(m_uVertexCount * 2u * 3u);
	Vertices vWeights;
	vWeights.Reserve(m_uVertexCount * 3u);
	BoneCounts vBoneCounts;
	vBoneCounts.Reserve(m_uVertexCount * 3u);

	UInt32 const uVertices = m_vVertices.GetSize();
	for (auto i = 0u; i < uVertices; )
	{
		auto const f = m_vVertices[i++];
		UInt16 const uBoneCount = (UInt16)((Int32)f);

		vBoneCounts.PushBack(uBoneCount);
		for (UInt32 j = i + (UInt32)uBoneCount * 4u; i < j; i += 4u)
		{
			vBoneCounts.PushBack((UInt16)((Int32)m_vVertices[i]));
			vVertices.PushBack(m_vVertices[i + 1u]);
			vVertices.PushBack(m_vVertices[i + 2u]);
			vWeights.PushBack(m_vVertices[i + 3u]);
		}
	}

	// Replace and complete.
	m_vBoneCounts.Swap(vBoneCounts);
	m_vVertices.Swap(vVertices);
	m_vWeights.Swap(vWeights);
	return true;
}

} // namespace Animation2D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
