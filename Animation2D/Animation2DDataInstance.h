/**
 * \file Animation2DDataInstance.h
 * \brief Mutable container of per-frame animation data state. Used to
 * capture an instance pose for query and rendering.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_DATA_INSTANCE_H
#define ANIMATION2D_DATA_INSTANCE_H

#include "Delegate.h"
#include "HashTable.h"
#include "Matrix2x3.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
#include "StandardVertex2D.h"
#include "Vector.h"
#include "Vector2D.h"
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation2D { struct Cache; } }
namespace Seoul { namespace Animation2D { class DataDefinition; } }
namespace Seoul { namespace Animation2D { struct BoneDefinition; } }
namespace Seoul { namespace Animation2D { struct IkDefinition; } }
namespace Seoul { namespace Animation2D { class PathAttachment; } }
namespace Seoul { namespace Animation2D { struct PathDefinition; } }
namespace Seoul { namespace Animation2D { struct SlotDataDefinition; } }
namespace Seoul { namespace Animation2D { struct TransformConstraintDefinition; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

namespace Animation2D
{

struct BoneInstance SEOUL_SEALED
{
	BoneInstance()
		: m_fPositionX(0.0f)
		, m_fPositionY(0.0f)
		, m_fRotationInDegrees(0.0f)
		, m_fScaleX(1.0f)
		, m_fScaleY(1.0f)
		, m_fShearX(0.0f)
		, m_fShearY(0.0f)
	{
	}

	Float32 m_fPositionX;
	Float32 m_fPositionY;
	Float32 m_fRotationInDegrees;
	Float32 m_fScaleX;
	Float32 m_fScaleY;
	Float32 m_fShearX;
	Float32 m_fShearY;

	BoneInstance& Assign(const BoneDefinition& data);

	static void ComputeWorldTransform(
		Float fPositionX,
		Float fPositionY,
		Float fRotationInDegrees,
		Float fScaleX,
		Float fScaleY,
		Float fShearX,
		Float fShearY,
		Matrix2x3& r)
	{
		Float32 const fRotationRad = DegreesToRadians(fRotationInDegrees + fShearX);
		Float32 const fRotationYRad = DegreesToRadians(fRotationInDegrees + 90.0f + fShearY);
		r.M00 = Cos(fRotationRad) * fScaleX;
		r.M01 = Cos(fRotationYRad) * fScaleY;
		r.M10 = Sin(fRotationRad) * fScaleX;
		r.M11 = Sin(fRotationYRad) * fScaleY;
		r.TX = fPositionX;
		r.TY = fPositionY;
	}

	void ComputeWorldTransform(Matrix2x3& r) const
	{
		ComputeWorldTransform(
			m_fPositionX,
			m_fPositionY,
			m_fRotationInDegrees,
			m_fScaleX,
			m_fScaleY,
			m_fShearX,
			m_fShearY,
			r);
	}
}; // struct BoneInstance

struct DeformKey SEOUL_SEALED
{
	DeformKey(HString skinId = HString(), HString slotId = HString(), HString attachmentId = HString())
		: m_SkinId(skinId)
		, m_SlotId(slotId)
		, m_AttachmentId(attachmentId)
	{
	}

	Bool operator==(const DeformKey& b) const
	{
		return (
			m_SkinId == b.m_SkinId &&
			m_SlotId == b.m_SlotId &&
			m_AttachmentId == b.m_AttachmentId);
	}

	Bool operator!=(const DeformKey& b) const
	{
		return !(*this == b);
	}

	UInt32 GetHash() const
	{
		UInt32 uReturn = 0u;
		IncrementalHash(uReturn, m_SkinId.GetHash());
		IncrementalHash(uReturn, m_SlotId.GetHash());
		IncrementalHash(uReturn, m_AttachmentId.GetHash());
		return uReturn;
	}

	HString m_SkinId;
	HString m_SlotId;
	HString m_AttachmentId;
}; // struct DeformKey
static inline UInt32 GetHash(const DeformKey& key)
{
	return key.GetHash();
}

struct IkInstance SEOUL_SEALED
{
	IkInstance()
		: m_fMix(1.0f)
		, m_fSoftness(0.0f)
		, m_bBendPositive(true)
		, m_bCompress(false)
		, m_bStretch(false)
		, m_bUniform(false)
	{
	}

	IkInstance& Assign(const IkDefinition& data);

	Float m_fMix;
	Float m_fSoftness;
	Bool m_bBendPositive;
	Bool m_bCompress;
	Bool m_bStretch;
	Bool m_bUniform;
}; // struct IkInstance

struct PathInstance SEOUL_SEALED
{
	typedef Vector<Float32, MemoryBudgets::Animation2D> Floats;
	typedef FixedArray<Float32, 10> Segments;

	PathInstance()
		: m_vCurves()
		, m_vLengths()
		, m_vPositions()
		, m_aSegments()
		, m_vSpaces()
		, m_vWorld()
		, m_fPosition(0.0f)
		, m_fPositionMix(1.0f)
		, m_fRotationMix(1.0f)
		, m_fSpacing(0.0f)
	{
	}

	PathInstance& Assign(const PathDefinition& data);

	Floats m_vCurves;
	Floats m_vLengths;
	Floats m_vPositions;
	Segments m_aSegments;
	Floats m_vSpaces;
	Floats m_vWorld;
	Float32 m_fPosition;
	Float32 m_fPositionMix;
	Float32 m_fRotationMix;
	Float32 m_fSpacing;
}; // struct PathInstance

struct SlotInstance SEOUL_SEALED
{
	SlotInstance(HString attachmentId = HString(), RGBA color = RGBA::White())
		: m_AttachmentId(attachmentId)
		, m_Color(color)
	{
	}

	SlotInstance& Assign(const SlotDataDefinition& data);

	Bool operator==(const SlotInstance& b) const
	{
		return (
			m_AttachmentId == b.m_AttachmentId &&
			m_Color == b.m_Color);
	}

	HString m_AttachmentId;
	RGBA m_Color;
}; // struct SlotInstance

struct TransformConstraintInstance SEOUL_SEALED
{
	TransformConstraintInstance()
		: m_fPositionMix(1.0f)
		, m_fRotationMix(1.0f)
		, m_fScaleMix(1.0f)
		, m_fShearMix(1.0f)
	{
	}

	TransformConstraintInstance& Assign(const TransformConstraintDefinition& data);

	Float m_fPositionMix;
	Float m_fRotationMix;
	Float m_fScaleMix;
	Float m_fShearMix;
}; // struct TransformConstraintInstance

} // namespace Animation2D
template <> struct CanMemCpy<Animation2D::BoneInstance> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::IkInstance> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::SlotInstance> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::TransformConstraintInstance> { static const Bool Value = true; };

template <>
struct DefaultHashTableKeyTraits<Animation2D::DeformKey>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Animation2D::DeformKey GetNullKey()
	{
		return Animation2D::DeformKey();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace Animation2D
{

class DataInstance SEOUL_SEALED
{
public:
	typedef Vector<BoneInstance, MemoryBudgets::Animation2D> BoneInstances;
	typedef Vector<Float, MemoryBudgets::Animation2D> DeformData;
	typedef HashTable<DeformKey, CheckedPtr<DeformData>, MemoryBudgets::Animation2D> Deforms;
	typedef HashTable<DeformKey, Int32, MemoryBudgets::Animation2D> DeformReferences;
	typedef Vector<Int16, MemoryBudgets::Animation2D> DrawOrder;
	typedef Vector<IkInstance, MemoryBudgets::Animation2D> IkInstances;
	typedef Vector<PathInstance, MemoryBudgets::Animation2D> PathInstances;
	typedef Vector<Matrix2x3, MemoryBudgets::Animation2D> SkinningPalette;
	typedef Vector<SlotInstance, MemoryBudgets::Animation2D> SlotInstances;
	typedef Vector<TransformConstraintInstance, MemoryBudgets::Animation2D> TransformConstraintStates;

	DataInstance(const SharedPtr<DataDefinition const>& pData, const SharedPtr<Animation::EventInterface>& pEventInterface);
	~DataInstance();

	DataInstance* Clone() const;

	const BoneInstances& GetBones() const { return m_vBones; }
	BoneInstances& GetBones() { return m_vBones; }

	// Return the animation accumulator cache owned by this NetworkInstance.
	const Cache& GetCache() const { return *m_pCache; }
	Cache& GetCache() { return *m_pCache; }

	const SharedPtr<Animation::EventInterface>& GetEventInterface() const { return m_pEventInterface; }
	const SharedPtr<DataDefinition const>& GetData() const { return m_pData; }

	const Deforms& GetDeforms() const { return m_tDeforms; }
	Deforms& GetDeforms() { return m_tDeforms; }

	const DeformReferences& GetDeformReferences() const { return m_tDeformReferences; }
	DeformReferences& GetDeformReferences() { return m_tDeformReferences; }

	const DrawOrder& GetDrawOrder() const { return m_vDrawOrder; }

	const IkInstances& GetIk() const { return m_vIk; }
	IkInstances& GetIk() { return m_vIk; }

	const PathInstances& GetPaths() const { return m_vPaths; }
	PathInstances& GetPaths() { return m_vPaths; }

	const SkinningPalette& GetSkinningPalette() const { return m_vSkinningPalette; }

	const SlotInstances& GetSlots() const { return m_vSlots; }
	SlotInstances& GetSlots() { return m_vSlots; }

	const TransformConstraintStates& GetTransformConstraintStates() const { return m_vTransformConstraintStates; }
	TransformConstraintStates& GetTransformConstraintStates() { return m_vTransformConstraintStates; }

	// Apply the current state of the animation cache to the instance state. This also resets the cache.
	void ApplyCache();

	// Prepare the skinning palette state of this instance for query and render.
	// Applies any animation changes made until now to the active skinning
	// palette.
	void PoseSkinningPalette();

private:
	ScopedPtr<Cache> const m_pCache;
	SharedPtr<DataDefinition const> const m_pData;
	SharedPtr<Animation::EventInterface> const m_pEventInterface;
	BoneInstances m_vBones;
	Deforms m_tDeforms;
	DeformReferences m_tDeformReferences;
	DrawOrder m_vDrawOrder;
	IkInstances m_vIk;
	PathInstances m_vPaths;
	SkinningPalette m_vSkinningPalette;
	SlotInstances m_vSlots;
	TransformConstraintStates m_vTransformConstraintStates;

	void InternalConstruct();
	SharedPtr<PathAttachment const> InternalGetPathAttachment(Int16 iTarget) const;
	void InternalPoseBone(Int16 iBone);
	void InternalPoseBone(
		Int16 iBone,
		Float fPositionX,
		Float fPositionY,
		Float fRotationInDegrees,
		Float fScaleX,
		Float fScaleY,
		Float fShearX,
		Float fShearY);
	void InternalPoseIk(Int16 iIk);
	void InternalPoseIk1(Int16 iParent, const Vector2D& vTarget, Float fAlpha, Bool bCompress, Bool bStretch, Bool bUniform);
	void InternalPoseIk2(Int16 iParent, Int16 iChild, const Vector2D& vTarget, Float fAlpha, Float fBendDirection, Bool bStretch, Float fSoftness);
	void InternalPosePathConstraint(Int16 iPath);
	Float32 const* InternalPosePathConstraintPoints(
		Int16 iPath,
		const SharedPtr<PathAttachment const>& pPathAttachment,
		UInt32 uSpaces,
		Bool bTangents,
		Bool bPosition,
		Bool bSpacing);
	void InternalPoseTransformConstraint(Int16 iTransform);
	void InternalPoseTransformConstraintAbsoluteWorld(Int16 iTransform);
	void InternalPoseTransformConstraintRelativeWorld(Int16 iTransform);
	void InternalPoseTransformConstraintAbsoluteLocal(Int16 iTransform);
	void InternalPoseTransformConstraintRelativeLocal(Int16 iTransform);

	SEOUL_DISABLE_COPY(DataInstance);
}; // class DataInstance

} // namespace Animation2D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
