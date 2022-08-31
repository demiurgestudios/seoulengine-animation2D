/**
 * \file Animation2DClipDefinition.h
 * \brief Contains a set of timelines that can be applied to
 * an Animation2DInstance, to pose its skeleton into a current state.
 * This is read-only data. To apply a Clip at runtime, you must
 * instantiate an Animation2D::ClipInstance.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_CLIP_DEFINITION_H
#define ANIMATION2D_CLIP_DEFINITION_H

#include "FixedArray.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"
#include "SharedPtr.h"
#include "StandardVertex2D.h"
#include "Vector.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { namespace Animation2D { class DataInstance; } }
namespace Seoul { namespace Animation2D { class ReadWriteUtil; } }
namespace Seoul { namespace Reflection { struct SerializeContext; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

namespace Animation2D
{

struct DrawOrderOffset SEOUL_SEALED
{
	DrawOrderOffset()
		: m_Slot()
		, m_iOffset(-1)
	{
	}

	Bool operator==(const DrawOrderOffset& b) const
	{
		return
			(m_Slot == b.m_Slot) &&
			(m_iOffset == b.m_iOffset);
	}

	Bool operator!=(const DrawOrderOffset& b) const
	{
		return !(*this == b);
	}

	HString m_Slot;
	Int16 m_iOffset;
}; // struct DrawOrderOffset

} // namespace Animation2D

template <> struct CanMemCpy<Animation2D::DrawOrderOffset> { static const Bool Value = true; };

namespace Animation2D
{

enum class CurveType : UInt32
{
	kLinear,
	kStepped,
	kBezier,
};

// Non-virtual by design (these are simple structs used
// in great quantities, and cache usage is a critical
// consideration). Don't use BaseKeyFrame directly, always
// use the subclasses (treat BaseKeyFrame as a mixin).
struct BaseKeyFrame
{
	BaseKeyFrame()
		: m_fTime(0.0f)
		, m_uCurveType((UInt32)CurveType::kLinear)
		, m_uCurveDataOffset(0u)
	{
	}

	Bool operator==(const BaseKeyFrame& b) const
	{
		return
			(m_fTime == b.m_fTime) &&
			(m_uCurveType == b.m_uCurveType) &&
			(m_uCurveDataOffset == b.m_uCurveDataOffset);
	}

	Bool operator!=(const BaseKeyFrame& b) const
	{
		return !(*this == b);
	}

	CurveType GetCurveType() const { return (CurveType)m_uCurveType; }
	void SetCurveType(CurveType eType) { m_uCurveType = (UInt32)eType; }

	Bool CustomDeserializeCurve(
		Reflection::SerializeContext* pContext,
		DataStore const* pDataStore,
		const DataNode& value,
		const DataNode& parentValue);

	Float32 m_fTime;
	UInt32 m_uCurveType : 2;
	UInt32 m_uCurveDataOffset : 30;
}; // struct BaseKeyFrame
SEOUL_STATIC_ASSERT(sizeof(BaseKeyFrame) == 8);

struct KeyFrame2D SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrame2D()
		: m_fX(0.0f)
		, m_fY(0.0f)
	{
	}

	Bool operator==(const KeyFrame2D& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fX == b.m_fX) &&
			(m_fY == b.m_fY);
	}

	Bool operator!=(const KeyFrame2D& b) const
	{
		return !(*this == b);
	}

	Float32 m_fX;
	Float32 m_fY;
}; // struct KeyFrame2D
SEOUL_STATIC_ASSERT(sizeof(KeyFrame2D) == 16);

struct KeyFrameDeform SEOUL_SEALED : public BaseKeyFrame
{
	typedef Vector<Float, MemoryBudgets::Animation2D> Vertices;

	KeyFrameDeform()
		: m_vVertices()
	{
	}

	Bool operator==(const KeyFrameDeform& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_vVertices == b.m_vVertices);
	}

	Bool operator!=(const KeyFrameDeform& b) const
	{
		return !(*this == b);
	}

	Vertices m_vVertices;

	static Bool CustomDeserializeType(
		Reflection::SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);
}; // struct KeyFrameDeform

struct KeyFramePathMix SEOUL_SEALED : public BaseKeyFrame
{
	KeyFramePathMix()
		: m_fPositionMix(1.0f)
		, m_fRotationMix(1.0f)
	{
	}

	Bool operator==(const KeyFramePathMix& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fPositionMix == b.m_fPositionMix) &&
			(m_fRotationMix == b.m_fRotationMix);
	}

	Bool operator!=(const KeyFramePathMix& b) const
	{
		return !(*this == b);
	}

	Float32 m_fPositionMix;
	Float32 m_fRotationMix;
}; // struct KeyFramePathMix
SEOUL_STATIC_ASSERT(sizeof(KeyFramePathMix) == 16);

struct KeyFramePathPosition SEOUL_SEALED : public BaseKeyFrame
{
	KeyFramePathPosition()
		: m_fPosition(0.0f)
	{
	}

	Bool operator==(const KeyFramePathPosition& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fPosition == b.m_fPosition);
	}

	Bool operator!=(const KeyFramePathPosition& b) const
	{
		return !(*this == b);
	}

	Float32 m_fPosition;
}; // struct KeyFramePathPosition
SEOUL_STATIC_ASSERT(sizeof(KeyFramePathPosition) == 12);

struct KeyFramePathSpacing SEOUL_SEALED : public BaseKeyFrame
{
	KeyFramePathSpacing()
		: m_fSpacing(0.0f)
	{
	}

	Bool operator==(const KeyFramePathSpacing& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fSpacing == b.m_fSpacing);
	}

	Bool operator!=(const KeyFramePathSpacing& b) const
	{
		return !(*this == b);
	}

	Float32 m_fSpacing;
}; // struct KeyFramePathSpacing
SEOUL_STATIC_ASSERT(sizeof(KeyFramePathSpacing) == 12);

struct KeyFrameRotation SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameRotation()
		: m_fAngleInDegrees(0.0f)
	{
	}

	Bool operator==(const KeyFrameRotation& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fAngleInDegrees == b.m_fAngleInDegrees);
	}

	Bool operator!=(const KeyFrameRotation& b) const
	{
		return !(*this == b);
	}

	Float32 m_fAngleInDegrees;
}; // struct KeyFrameRotation
SEOUL_STATIC_ASSERT(sizeof(KeyFrameRotation) == 12);

struct KeyFrameScale SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameScale()
		: m_fX(1.0f)
		, m_fY(1.0f)
	{
	}

	Bool operator==(const KeyFrameScale& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fX == b.m_fX) &&
			(m_fY == b.m_fY);
	}

	Bool operator!=(const KeyFrameScale& b) const
	{
		return !(*this == b);
	}

	Float32 m_fX;
	Float32 m_fY;
}; // struct KeyFrameScale
SEOUL_STATIC_ASSERT(sizeof(KeyFrameScale) == 16);

struct KeyFrameAttachment SEOUL_SEALED
{
	KeyFrameAttachment()
		: m_fTime(0.0f)
		, m_Id()

	{
	}

	Bool operator==(const KeyFrameAttachment& b) const
	{
		return
			(m_fTime == b.m_fTime) &&
			(m_Id == b.m_Id);
	}

	Bool operator!=(const KeyFrameAttachment& b) const
	{
		return !(*this == b);
	}

	Float32 m_fTime;
	HString m_Id;
}; // struct KeyFrameAttachment
SEOUL_STATIC_ASSERT(sizeof(KeyFrameAttachment) == 8);

struct KeyFrameColor SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameColor()
		: m_Color(RGBA::White())
	{
	}

	Bool operator==(const KeyFrameColor& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_Color == b.m_Color);
	}

	Bool operator!=(const KeyFrameColor& b) const
	{
		return !(*this == b);
	}

	RGBA m_Color;
}; // struct KeyFrameColor
SEOUL_STATIC_ASSERT(sizeof(KeyFrameColor) == 12);

struct KeyFrameDrawOrder SEOUL_SEALED
{
	typedef Vector<DrawOrderOffset, MemoryBudgets::Animation2D> Offsets;

	KeyFrameDrawOrder()
		: m_fTime(0.0f)
		, m_vOffsets()

	{
	}

	Bool operator==(const KeyFrameDrawOrder& b) const
	{
		return
			(m_fTime == b.m_fTime) &&
			(m_vOffsets == b.m_vOffsets);
	}

	Bool operator!=(const KeyFrameDrawOrder& b) const
	{
		return !(*this == b);
	}

	Float32 m_fTime;
	Offsets m_vOffsets;
}; // struct KeyFrameDrawOrder

struct KeyFrameEvent SEOUL_SEALED
{
	KeyFrameEvent()
		: m_fTime(0.0f)
		, m_f(0.0f)
		, m_i(0)
		, m_s()
		, m_Id()

	{
	}

	Bool operator==(const KeyFrameEvent& b) const
	{
		return
			(m_fTime == b.m_fTime) &&
			(m_f == b.m_f) &&
			(m_i == b.m_i) &&
			(m_s == b.m_s) &&
			(m_Id == b.m_Id);
	}

	Bool operator!=(const KeyFrameEvent& b) const
	{
		return !(*this == b);
	}

	Float32 m_fTime;
	Float32 m_f;
	Int32 m_i;
	String m_s;
	HString m_Id;

	static Bool CustomDeserializeType(
		Reflection::SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);
}; // struct KeyFrameEvent

struct KeyFrameIk SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameIk()
		: m_fMix(1.0f)
		, m_fSoftness(0.0f)
		, m_bBendPositive(true)
		, m_bCompress(false)
		, m_bStretch(false)
	{
	}

	Bool operator==(const KeyFrameIk& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fMix == b.m_fMix) &&
			(m_fSoftness == b.m_fSoftness) &&
			(m_bBendPositive == b.m_bBendPositive) &&
			(m_bCompress == b.m_bCompress) &&
			(m_bStretch == b.m_bStretch);
	}

	Bool operator!=(const KeyFrameIk& b) const
	{
		return !(*this == b);
	}

	Float32 m_fMix;
	Float32 m_fSoftness;
	Bool m_bBendPositive;
	Bool m_bCompress;
	Bool m_bStretch;
}; // struct KeyFrameIk
SEOUL_STATIC_ASSERT(sizeof(KeyFrameIk) == 20);

struct KeyFrameTransform SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameTransform()
		: m_fPositionMix(1.0f)
		, m_fRotationMix(1.0f)
		, m_fScaleMix(1.0f)
		, m_fShearMix(1.0f)
	{
	}

	Bool operator==(const KeyFrameTransform& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_fPositionMix == b.m_fPositionMix) &&
			(m_fRotationMix == b.m_fRotationMix) &&
			(m_fScaleMix == b.m_fScaleMix) &&
			(m_fShearMix == b.m_fShearMix);
	}

	Bool operator!=(const KeyFrameTransform& b) const
	{
		return !(*this == b);
	}

	Float32 m_fPositionMix;
	Float32 m_fRotationMix;
	Float32 m_fScaleMix;
	Float32 m_fShearMix;
}; // struct KeyFrameTransform
SEOUL_STATIC_ASSERT(sizeof(KeyFrameTransform) == 24);

struct KeyFrameTwoColor SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameTwoColor()
		: m_Color(RGBA::White())
		, m_SecondaryColor(RGBA::White())
	{
	}

	Bool operator==(const KeyFrameTwoColor& b) const
	{
		return
			*((BaseKeyFrame const*)this) == *((BaseKeyFrame const*)&b) &&
			(m_Color == b.m_Color) &&
			(m_SecondaryColor == b.m_SecondaryColor);
	}

	Bool operator!=(const KeyFrameTwoColor& b) const
	{
		return !(*this == b);
	}

	RGBA m_Color;
	RGBA m_SecondaryColor;
}; // struct KeyFrameTwoColor
SEOUL_STATIC_ASSERT(sizeof(KeyFrameTwoColor) == 16);

} // namespace Animation2D

template <> struct CanMemCpy<Animation2D::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrame2D> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameColor> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameIk> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFramePathMix> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFramePathPosition> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFramePathSpacing> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameRotation> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameScale> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameTransform> { static const Bool Value = true; };
template <> struct CanMemCpy<Animation2D::KeyFrameTwoColor> { static const Bool Value = true; };

template <> struct CanZeroInit<Animation2D::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::KeyFrame2D> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::KeyFramePathPosition> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::KeyFramePathSpacing> { static const Bool Value = true; };
template <> struct CanZeroInit<Animation2D::KeyFrameRotation> { static const Bool Value = true; };

namespace Animation2D
{

typedef Vector<KeyFrame2D, MemoryBudgets::Animation2D> KeyFrames2D;
typedef Vector<KeyFrameAttachment, MemoryBudgets::Animation2D> KeyFramesAttachment;
typedef Vector<KeyFrameColor, MemoryBudgets::Animation2D> KeyFramesColor;
typedef Vector<KeyFrameDeform, MemoryBudgets::Animation2D> KeyFramesDeform;
typedef Vector<KeyFrameDrawOrder, MemoryBudgets::Animation2D> KeyFramesDrawOrder;
typedef Vector<KeyFrameEvent, MemoryBudgets::Animation2D> KeyFramesEvent;
typedef Vector<KeyFrameIk, MemoryBudgets::Animation2D> KeyFramesIk;
typedef Vector<KeyFramePathMix, MemoryBudgets::Animation2D> KeyFramesPathMix;
typedef Vector<KeyFramePathPosition, MemoryBudgets::Animation2D> KeyFramesPathPosition;
typedef Vector<KeyFramePathSpacing, MemoryBudgets::Animation2D> KeyFramesPathSpacing;
typedef Vector<KeyFrameRotation, MemoryBudgets::Animation2D> KeyFramesRotation;
typedef Vector<KeyFrameScale, MemoryBudgets::Animation2D> KeyFramesScale;
typedef Vector<KeyFrameTransform, MemoryBudgets::Animation2D> KeyFramesTransform;
typedef Vector<KeyFrameTwoColor, MemoryBudgets::Animation2D> KeyFramesTwoColor;

static const UInt32 kuBezierCurvePoints = 18u;
typedef FixedArray<Float, kuBezierCurvePoints> BezierCurve;
typedef Vector<BezierCurve, MemoryBudgets::Animation2D> BezierCurves;

struct BoneKeyFrames SEOUL_SEALED
{
	BoneKeyFrames()
		: m_vRotation()
		, m_vScale()
		, m_vShear()
		, m_vTranslation()
	{
	}

	Bool operator==(const BoneKeyFrames& b) const
	{
		return
			(m_vRotation == b.m_vRotation) &&
			(m_vScale == b.m_vScale) &&
			(m_vShear == b.m_vShear) &&
			(m_vTranslation == b.m_vTranslation);
	}

	Bool operator!=(const BoneKeyFrames& b) const
	{
		return !(*this == b);
	}

	KeyFramesRotation m_vRotation;
	KeyFramesScale m_vScale;
	KeyFrames2D m_vShear;
	KeyFrames2D m_vTranslation;
}; // struct BoneKeyFrames

struct PathKeyFrames SEOUL_SEALED
{
	PathKeyFrames()
		: m_vMix()
		, m_vPosition()
		, m_vSpacing()
	{
	}

	Bool operator==(const PathKeyFrames& b) const
	{
		return
			(m_vMix == b.m_vMix) &&
			(m_vPosition == b.m_vPosition) &&
			(m_vSpacing == b.m_vSpacing);
	}

	Bool operator!=(const PathKeyFrames& b) const
	{
		return !(*this == b);
	}

	KeyFramesPathMix m_vMix;
	KeyFramesPathPosition m_vPosition;
	KeyFramesPathSpacing m_vSpacing;
}; // struct PathKeyFrames

struct SlotKeyFrames SEOUL_SEALED
{
	SlotKeyFrames()
		: m_vAttachment()
		, m_vColor()
		, m_vTwoColor()
	{
	}

	Bool operator==(const SlotKeyFrames& b) const
	{
		return
			(m_vAttachment == b.m_vAttachment) &&
			(m_vColor == b.m_vColor) &&
			(m_vTwoColor == b.m_vTwoColor);
	}

	Bool operator!=(const SlotKeyFrames& b) const
	{
		return !(*this == b);
	}

	KeyFramesAttachment m_vAttachment;
	KeyFramesColor m_vColor;
	KeyFramesTwoColor m_vTwoColor;
}; // struct SlotKeyFrames

class Clip SEOUL_SEALED
{
public:
	typedef HashTable<HString, BoneKeyFrames, MemoryBudgets::Animation2D> Bones;
	typedef HashTable<HString, HashTable<HString, HashTable<HString, KeyFramesDeform, MemoryBudgets::Animation2D>, MemoryBudgets::Animation2D>, MemoryBudgets::Animation2D> Deforms;
	typedef HashTable<HString, KeyFramesIk, MemoryBudgets::Animation2D> Ik;
	typedef HashTable<HString, PathKeyFrames, MemoryBudgets::Animation2D> Paths;
	typedef HashTable<HString, SlotKeyFrames, MemoryBudgets::Animation2D> Slots;
	typedef HashTable<HString, KeyFramesTransform, MemoryBudgets::Animation2D> Transforms;

	Clip();
	~Clip();

	// Direct to binary support.
	Bool Load(ReadWriteUtil& r);
	Bool Save(ReadWriteUtil& r) const;

	const Bones& GetBones() const { return m_tBones; }
	const Deforms& GetDeforms() const { return m_tDeforms; }
	const KeyFramesDrawOrder& GetDrawOrder() const { return m_vDrawOrder; }
	const KeyFramesEvent& GetEvents() const { return m_vEvents; }
	const Ik& GetIk() const { return m_tIk; }
	const Paths& GetPaths() const { return m_tPaths; }
	const Slots& GetSlots() const { return m_tSlots; }
	const Transforms& GetTransforms() const { return m_tTransforms; }

	Bool operator==(const Clip& b) const;

private:
	SEOUL_REFERENCE_COUNTED(Clip);
	SEOUL_REFLECTION_FRIENDSHIP(Clip);

	Bones m_tBones;
	Deforms m_tDeforms;
	KeyFramesDrawOrder m_vDrawOrder;
	KeyFramesEvent m_vEvents;
	Ik m_tIk;
	Paths m_tPaths;
	Slots m_tSlots;
	Transforms m_tTransforms;

	static Bool CustomDeserializeType(
		Reflection::SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);

	SEOUL_DISABLE_COPY(Clip);
}; // class Clip

} // namespace Animation2D

static inline Bool operator==(const SharedPtr<Animation2D::Clip>& pA, const SharedPtr<Animation2D::Clip>& pB)
{
	if (pA.GetPtr() == pB.GetPtr()) { return true; }
	if (!pA.IsValid() || !pB.IsValid()) { return false; }

	return (*pA == *pB);
}

static inline Bool operator!=(const SharedPtr<Animation2D::Clip>& pA, const SharedPtr<Animation2D::Clip>& pB)
{
	return !(pA == pB);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
