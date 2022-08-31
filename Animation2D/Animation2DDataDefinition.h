/**
 * \file Animation2DDataDefinition.h
 * \brief Serializable animation data. Includes skeleton and bones,
 * attachments, event data, and animation clip data. This is read-only
 * data at runtime, you must instantiate an Animation2D::DataInstance
 * to create runtime animation state.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_DATA_DEFINITION_H
#define ANIMATION2D_DATA_DEFINITION_H

#include "Animation2DAttachment.h"
#include "Animation2DClipDefinition.h"
#include "ContentHandle.h"
#include "ContentTraits.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SeoulHString.h"
#include "SharedPtr.h"
#include "StandardVertex2D.h"
#include "Vector.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

namespace Animation2D
{

// Current expected spine version.
static const String ksExpectedSpineVersion("3.8.79");

enum class SlotBlendMode : UInt32
{
	kAlpha,
	kAdditive,
	kMultiply, // TODO: Tracked but currently unsupported.
	kScreen, // TODO: Tracked but currently unsupported.
};

enum class TransformMode
{
	kNormal,
	kOnlyTranslation,
	kNoRotationOrReflection,
	kNoScale,
	kNoScaleOrReflection,
};

struct BoneDefinition SEOUL_SEALED
{
	BoneDefinition()
		: m_Id()
		, m_ParentId()
		, m_fLength(0.0f)
		, m_fPositionX(0.0f)
		, m_fPositionY(0.0f)
		, m_fRotationInDegrees(0.0f)
		, m_fScaleX(1.0f)
		, m_fScaleY(1.0f)
		, m_fShearX(0.0f)
		, m_fShearY(0.0f)
		, m_eTransformMode(TransformMode::kNormal)
		, m_iParent(-1)
		, m_bSkinRequired(false)
	{
	}

	Bool operator==(const BoneDefinition& b) const
	{
		return
			(m_Id == b.m_Id) &&
			(m_ParentId == b.m_ParentId) &&
			(m_fLength == b.m_fLength) &&
			(m_fPositionX == b.m_fPositionX) &&
			(m_fPositionY == b.m_fPositionY) &&
			(m_fRotationInDegrees == b.m_fRotationInDegrees) &&
			(m_fScaleX == b.m_fScaleX) &&
			(m_fScaleY == b.m_fScaleY) &&
			(m_fShearX == b.m_fShearX) &&
			(m_fShearY == b.m_fShearY) &&
			(m_eTransformMode == b.m_eTransformMode) &&
			(m_iParent == b.m_iParent) &&
			(m_bSkinRequired == b.m_bSkinRequired);
	}

	Bool operator!=(const BoneDefinition& b) const
	{
		return !(*this == b);
	}

	HString m_Id;
	HString m_ParentId;
	Float32 m_fLength;
	Float32 m_fPositionX;
	Float32 m_fPositionY;
	Float32 m_fRotationInDegrees;
	Float32 m_fScaleX;
	Float32 m_fScaleY;
	Float32 m_fShearX;
	Float32 m_fShearY;
	TransformMode m_eTransformMode;
	Int16 m_iParent;
	Bool m_bSkinRequired;
}; // struct BoneDefinition

struct EventDefinition SEOUL_SEALED
{
	EventDefinition()
		: m_f(0.0f)
		, m_i(0)
		, m_s()
	{
	}

	Bool operator==(const EventDefinition& b) const
	{
		return
			(m_f == b.m_f) &&
			(m_i == b.m_i) &&
			(m_s == b.m_s);
	}

	Bool operator!=(const EventDefinition& b) const
	{
		return !(*this == b);
	}

	Float32 m_f;
	Int32 m_i;
	String m_s;
}; // struct EventDefinition

struct IkDefinition SEOUL_SEALED
{
	IkDefinition()
		: m_vBoneIds()
		, m_viBones()
		, m_Id()
		, m_TargetId()
		, m_fMix(1.0f)
		, m_fSoftness(0.0f)
		, m_iOrder(0)
		, m_iTarget(-1)
		, m_bBendPositive(true)
		, m_bSkinRequired(false)
		, m_bCompress(false)
		, m_bStretch(false)
		, m_bUniform(false)
	{
	}

	Bool operator==(const IkDefinition& b) const
	{
		return
			(m_vBoneIds == b.m_vBoneIds) &&
			(m_viBones == b.m_viBones) &&
			(m_Id == b.m_Id) &&
			(m_TargetId == b.m_TargetId) &&
			(m_fMix == b.m_fMix) &&
			(m_fSoftness == b.m_fSoftness) &&
			(m_iOrder == b.m_iOrder) &&
			(m_iTarget == b.m_iTarget) &&
			(m_bBendPositive == b.m_bBendPositive) &&
			(m_bSkinRequired == b.m_bSkinRequired) &&
			(m_bCompress == b.m_bCompress) &&
			(m_bStretch == b.m_bStretch) &&
			(m_bUniform == b.m_bUniform);
	}

	Bool operator!=(const IkDefinition& b) const
	{
		return !(*this == b);
	}

	typedef Vector<HString, MemoryBudgets::Animation2D> BoneIds;
	typedef Vector<Int16, MemoryBudgets::Animation2D> BoneIndices;
	BoneIds m_vBoneIds;
	BoneIndices m_viBones;
	HString m_Id;
	HString m_TargetId;
	Float m_fMix;
	Float m_fSoftness;
	Int32 m_iOrder;
	Int16 m_iTarget;
	Bool m_bBendPositive;
	Bool m_bSkinRequired;
	Bool m_bCompress;
	Bool m_bStretch;
	Bool m_bUniform;
}; // struct IkDefinition

struct MetaData SEOUL_SEALED
{
	MetaData()
		: m_fPositionX(0.0f)
		, m_fPositionY(0.0f)
		, m_fFPS(30.0f)
		, m_fHeight(0.0f)
		, m_fWidth(0.0f)
	{
	}

	Float32 m_fPositionX;
	Float32 m_fPositionY;
	Float32 m_fFPS;
	Float32 m_fHeight;
	Float32 m_fWidth;

	Bool operator==(const MetaData& b) const
	{
		return
			(m_fPositionX == b.m_fPositionX) &&
			(m_fPositionY == b.m_fPositionY) &&
			(m_fFPS == b.m_fFPS) &&
			(m_fHeight == b.m_fHeight) &&
			(m_fWidth == b.m_fWidth);
	}

	Bool operator!=(const MetaData& b) const
	{
		return !(*this == b);
	}
}; // struct MetaData

enum class PathPositionMode : UInt32
{
	kPercent,
	kFixed,
};

enum class PathRotationMode : UInt32
{
	kTangent,
	kChain,
	kChainScale,
};

enum class PathSpacingMode : UInt32
{
	kLength,
	kFixed,
	kPercent,
};

struct PathDefinition SEOUL_SEALED
{
	PathDefinition()
		: m_vBoneIds()
		, m_viBones()
		, m_Id()
		, m_fPosition(0.0f)
		, m_fPositionMix(1.0f)
		, m_ePositionMode(PathPositionMode::kPercent)
		, m_fRotationInDegrees(0.0f)
		, m_fRotationMix(1.0f)
		, m_eRotationMode(PathRotationMode::kTangent)
		, m_fSpacing(0.0f)
		, m_eSpacingMode(PathSpacingMode::kLength)
		, m_TargetId()
		, m_iOrder(0)
		, m_iTarget(-1)
		, m_bSkinRequired(false)
	{
	}

	Bool operator==(const PathDefinition& b) const
	{
		return
			(m_vBoneIds == b.m_vBoneIds) &&
			(m_viBones == b.m_viBones) &&
			(m_Id == b.m_Id) &&
			(m_fPosition == b.m_fPosition) &&
			(m_fPositionMix == b.m_fPositionMix) &&
			(m_ePositionMode == b.m_ePositionMode) &&
			(m_fRotationInDegrees == b.m_fRotationInDegrees) &&
			(m_fRotationMix == b.m_fRotationMix) &&
			(m_eRotationMode == b.m_eRotationMode) &&
			(m_fSpacing == b.m_fSpacing) &&
			(m_eSpacingMode == b.m_eSpacingMode) &&
			(m_TargetId == b.m_TargetId) &&
			(m_iOrder == b.m_iOrder) &&
			(m_iTarget == b.m_iTarget) &&
			(m_bSkinRequired == b.m_bSkinRequired);
	}

	Bool operator!=(const PathDefinition& b) const
	{
		return !(*this == b);
	}

	typedef Vector<HString, MemoryBudgets::Animation2D> BoneIds;
	typedef Vector<Int16, MemoryBudgets::Animation2D> BoneIndices;
	BoneIds m_vBoneIds;
	BoneIndices m_viBones;
	HString m_Id;
	Float32 m_fPosition;
	Float32 m_fPositionMix;
	PathPositionMode m_ePositionMode;
	Float32 m_fRotationInDegrees;
	Float32 m_fRotationMix;
	PathRotationMode m_eRotationMode;
	Float32 m_fSpacing;
	PathSpacingMode m_eSpacingMode;
	HString m_TargetId;
	Int32 m_iOrder;
	Int16 m_iTarget;
	Bool m_bSkinRequired;
}; // struct PathDefinition

enum class PoseTaskType : Int16
{
	kBone,
	kIk,
	kPath,
	kTransform,
};

struct PoseTask SEOUL_SEALED
{
	PoseTask(PoseTaskType eType = PoseTaskType::kBone, Int32 iIndex = -1)
		: m_iType((Int16)eType)
		, m_iIndex((Int16)iIndex)
	{
		// Sanity check for out of range values.
		SEOUL_ASSERT(iIndex <= Int16Max);
	}

	Bool operator==(const PoseTask& b) const
	{
		return
			(m_iType == b.m_iType) &&
			(m_iIndex == b.m_iIndex);
	}

	Bool operator!=(const PoseTask& b) const
	{
		return !(*this == b);
	}

	Int16 m_iType;
	Int16 m_iIndex;
}; // struct PoseTask

struct SlotDataDefinition SEOUL_SEALED
{
	SlotDataDefinition()
		: m_Id()
		, m_AttachmentId()
		, m_eBlendMode(SlotBlendMode::kAlpha)
		, m_Color(RGBA::White())
		, m_BoneId()
		, m_iBone(-1)
		, m_SecondaryColor(RGBA::Black())
		, m_bHasSecondaryColor(false)
	{
	}

	Bool operator==(const SlotDataDefinition& b) const
	{
		return
			(m_Id == b.m_Id) &&
			(m_AttachmentId == b.m_AttachmentId) &&
			(m_eBlendMode == b.m_eBlendMode) &&
			(m_Color == b.m_Color) &&
			(m_BoneId == b.m_BoneId) &&
			(m_iBone == b.m_iBone) &&
			(m_SecondaryColor == b.m_SecondaryColor) &&
			(m_bHasSecondaryColor == b.m_bHasSecondaryColor);
	}

	Bool operator!=(const SlotDataDefinition& b) const
	{
		return !(*this == b);
	}

	RGBA GetSecondaryColor() const { return m_SecondaryColor; }
	void SetSecondaryColor(RGBA color)
	{
		m_SecondaryColor = color;
		m_bHasSecondaryColor = true;
	}

	HString m_Id;
	HString m_AttachmentId;
	SlotBlendMode m_eBlendMode;
	RGBA m_Color;
	HString m_BoneId;
	Int16 m_iBone;
	RGBA m_SecondaryColor;
	Bool m_bHasSecondaryColor;
}; // struct SlotDataDefinition

struct TransformConstraintDefinition SEOUL_SEALED
{
	TransformConstraintDefinition()
		: m_vBoneIds()
		, m_viBones()
		, m_Id()
		, m_fDeltaPositionX(0.0f)
		, m_fDeltaPositionY(0.0f)
		, m_fDeltaRotationInDegrees(0.0f)
		, m_fDeltaScaleX(0.0f)
		, m_fDeltaScaleY(0.0f)
		, m_fDeltaShearY(0.0f)
		, m_fPositionMix(1.0f)
		, m_fRotationMix(1.0f)
		, m_fScaleMix(1.0f)
		, m_fShearMix(1.0f)
		, m_TargetId()
		, m_iOrder(0)
		, m_iTarget(-1)
		, m_bSkinRequired(false)
		, m_bLocal(false)
		, m_bRelative(false)
	{
	}

	Bool operator==(const TransformConstraintDefinition& b) const
	{
		return
			(m_vBoneIds == b.m_vBoneIds) &&
			(m_viBones == b.m_viBones) &&
			(m_Id == b.m_Id) &&
			(m_fDeltaPositionX == b.m_fDeltaPositionX) &&
			(m_fDeltaPositionY == b.m_fDeltaPositionY) &&
			(m_fDeltaRotationInDegrees == b.m_fDeltaRotationInDegrees) &&
			(m_fDeltaScaleX == b.m_fDeltaScaleX) &&
			(m_fDeltaScaleY == b.m_fDeltaScaleY) &&
			(m_fDeltaShearY == b.m_fDeltaShearY) &&
			(m_fPositionMix == b.m_fPositionMix) &&
			(m_fRotationMix == b.m_fRotationMix) &&
			(m_fScaleMix == b.m_fScaleMix) &&
			(m_fShearMix == b.m_fShearMix) &&
			(m_TargetId == b.m_TargetId) &&
			(m_iOrder == b.m_iOrder) &&
			(m_iTarget == b.m_iTarget) &&
			(m_bSkinRequired == b.m_bSkinRequired) &&
			(m_bLocal == b.m_bLocal) &&
			(m_bRelative == b.m_bRelative);
	}

	Bool operator!=(const TransformConstraintDefinition& b) const
	{
		return !(*this == b);
	}

	typedef Vector<HString, MemoryBudgets::Animation2D> BoneIds;
	typedef Vector<Int16, MemoryBudgets::Animation2D> BoneIndices;
	BoneIds m_vBoneIds;
	BoneIndices m_viBones;
	HString m_Id;
	Float32 m_fDeltaPositionX;
	Float32 m_fDeltaPositionY;
	Float32 m_fDeltaRotationInDegrees;
	Float32 m_fDeltaScaleX;
	Float32 m_fDeltaScaleY;
	Float32 m_fDeltaShearY;
	Float m_fPositionMix;
	Float m_fRotationMix;
	Float m_fScaleMix;
	Float m_fShearMix;
	HString m_TargetId;
	Int32 m_iOrder;
	Int16 m_iTarget;
	Bool m_bSkinRequired;
	Bool m_bLocal;
	Bool m_bRelative;
}; // struct TransformConstraintDefinition

class DataDefinition SEOUL_SEALED
{
public:
	typedef HashTable<HString, SharedPtr<Attachment>, MemoryBudgets::Animation2D> AttachmentSets;
	typedef HashTable<HString, AttachmentSets, MemoryBudgets::Animation2D> Attachments;
	typedef Vector<BoneDefinition, MemoryBudgets::Animation2D> Bones;
	typedef HashTable<HString, SharedPtr<Clip>, MemoryBudgets::Animation2D> Clips;
	typedef HashTable<HString, EventDefinition, MemoryBudgets::Animation2D> Events;
	typedef Vector<Float, MemoryBudgets::Animation2D> Floats;
	typedef Vector<IkDefinition, MemoryBudgets::Animation2D> Ik;
	typedef HashTable<HString, Int16, MemoryBudgets::Animation2D> Lookup;
	typedef Vector<PathDefinition, MemoryBudgets::Animation2D> Paths;
	typedef Vector<PoseTask, MemoryBudgets::Animation2D> PoseTasks;
	typedef HashTable<HString, Attachments, MemoryBudgets::Animation2D> Skins;
	typedef Vector<SlotDataDefinition, MemoryBudgets::Animation2D> Slots;
	typedef Vector<TransformConstraintDefinition, MemoryBudgets::Animation2D> Transforms;

	DataDefinition(FilePath filePath);
	~DataDefinition();

	// Direct to binary support.
	Bool Load(ReadWriteUtil& r);
	Bool Save(ReadWriteUtil& r) const;

	const Bones& GetBones() const { return m_vBones; }
	Int16 GetBoneIndex(HString id) const { Int16 i = -1; (void)m_tBones.GetValue(id, i); return i; }

	SharedPtr<Clip> GetClip(HString id) const { SharedPtr<Clip> p; (void)m_tClips.GetValue(id, p); return p; }
	const Clips& GetClips() const { return m_tClips; }

	const BezierCurves& GetCurves() const { return m_vCurves; }
	BezierCurves& GetCurves() { return m_vCurves; }

	const Events& GetEvents() const { return m_tEvents; }

	const Ik& GetIk() const { return m_vIk; }
	Int16 GetIkIndex(HString id) const { Int16 i = -1; (void)m_tIk.GetValue(id, i); return i; }

	const MetaData& GetMetaData() const { return m_MetaData; }

	const Paths& GetPaths() const { return m_vPaths; }
	Int16 GetPathIndex(HString id) const { Int16 i = -1; (void)m_tPaths.GetValue(id, i); return i; }

	const PoseTasks& GetPoseTasks() const { return m_vPoseTasks; }

	Bool GetAttachment(
		HString skinId,
		HString slotId,
		HString attachmentId,
		SharedPtr<Attachment>& rOutAttachment) const;

	const Skins& GetSkins() const { return m_tSkins; }

	const Slots& GetSlots() const { return m_vSlots; }
	Int16 GetSlotIndex(HString id) const { Int16 i = -1; (void)m_tSlots.GetValue(id, i); return i; }

	const Transforms& GetTransforms() const { return m_vTransforms; }
	Int16 GetTransformIndex(HString id) const { Int16 i = -1; (void)m_tTransforms.GetValue(id, i); return i; }

	Bool CopyBaseVertices(
		HString skinId,
		HString slotId,
		HString attachmentId,
		Floats& rv) const;

	Bool operator==(const DataDefinition& b) const;

private:
	SEOUL_REFERENCE_COUNTED(DataDefinition);
	SEOUL_REFLECTION_FRIENDSHIP(DataDefinition);

	FilePath const m_FilePath;
	Bones m_vBones;
	Lookup m_tBones;
	Clips m_tClips;
	BezierCurves m_vCurves;
	Events m_tEvents;
	Ik m_vIk;
	Lookup m_tIk;
	MetaData m_MetaData;
	Paths m_vPaths;
	Lookup m_tPaths;
	PoseTasks m_vPoseTasks;
	Skins m_tSkins;
	Slots m_vSlots;
	Lookup m_tSlots;
	Transforms m_vTransforms;
	Lookup m_tTransforms;

	Bool DeserializeSkin(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value);

	Bool FinalizeBones();
	Bool FinalizeIk();
	Bool FinalizePaths();
	Bool FinalizePoseTasks();
	Bool FinalizeSkins();
	Bool FinalizeSlots();
	Bool FinalizeTransforms();

	static Bool CustomDeserializeType(
		Reflection::SerializeContext& rContext,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::WeakAny& objectThis,
		Bool bSkipPostSerialize);

	SEOUL_DISABLE_COPY(DataDefinition);
}; // class DataDefinition

} // namespace Animation2D

typedef Content::Handle<Animation2D::DataDefinition> Animation2DDataContentHandle;

namespace Content
{

/**
 * Specialization of Content::Traits<> for Animation2D::DataDefinition, allows Animation2D::DataDefinition to be managed
 * as loadable content in the content system.
 */
template <>
struct Traits<Animation2D::DataDefinition>
{
	typedef FilePath KeyType;
	static const Bool CanSyncLoad = false;

	static SharedPtr<Animation2D::DataDefinition> GetPlaceholder(FilePath filePath);
	static Bool FileChange(FilePath filePath, const Animation2DDataContentHandle& hEntry);
	static void Load(FilePath filePath, const Animation2DDataContentHandle& hEntry);
	static Bool PrepareDelete(FilePath filePath, Entry<Animation2D::DataDefinition, KeyType>& entry);
	static void SyncLoad(FilePath filePath, const Handle<Animation2D::DataDefinition>& pEntry) {}
	static UInt32 GetMemoryUsage(const SharedPtr<Animation2D::DataDefinition>& p) { return 0u; }
}; // Content::Traits<Animation2D::DataDefinition>

} // namespace Content

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
