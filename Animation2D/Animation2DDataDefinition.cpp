/**
 * \file Animation2DDataDefinition.cpp
 * \brief Serializable animation data. Includes skeleton and bones,
 * attachments, event data, and animation clip data. This is read-only
 * data at runtime, you must instantiate an Animation2D::DataInstance
 * to create runtime animation state.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "Animation2DContentLoader.h"
#include "Animation2DDataDefinition.h"
#include "Animation2DReadWriteUtil.h"
#include "ContentLoadManager.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "StackOrHeapArray.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

static const HString kAttachments("attachments");
static const HString kName("name");

SEOUL_SPEC_TEMPLATE_TYPE(HashTable<HString, Animation2D::EventDefinition, 2, DefaultHashTableKeyTraits<HString>>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::BoneDefinition, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::IkDefinition, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::PathDefinition, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::SlotDataDefinition, 2>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::TransformConstraintDefinition, 2>)
SEOUL_BEGIN_ENUM(Animation2D::SlotBlendMode)
	SEOUL_ENUM_N("normal", Animation2D::SlotBlendMode::kAlpha)
	SEOUL_ENUM_N("additive", Animation2D::SlotBlendMode::kAdditive)
	SEOUL_ENUM_N("multiply", Animation2D::SlotBlendMode::kMultiply)
	SEOUL_ENUM_N("screen", Animation2D::SlotBlendMode::kScreen)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Animation2D::TransformMode)
	SEOUL_ENUM_N("normal", Animation2D::TransformMode::kNormal)
	SEOUL_ENUM_N("onlyTranslation", Animation2D::TransformMode::kOnlyTranslation)
	SEOUL_ENUM_N("noRotationOrReflection", Animation2D::TransformMode::kNoRotationOrReflection)
	SEOUL_ENUM_N("noScale", Animation2D::TransformMode::kNoScale)
	SEOUL_ENUM_N("noScaleOrReflection", Animation2D::TransformMode::kNoScaleOrReflection)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Animation2D::BoneDefinition)
	SEOUL_PROPERTY_N("name", m_Id)
	SEOUL_PROPERTY_N("parent", m_ParentId)
	SEOUL_PROPERTY_N("length", m_fLength)
	SEOUL_PROPERTY_N("x", m_fPositionX)
	SEOUL_PROPERTY_N("y", m_fPositionY)
	SEOUL_PROPERTY_N("rotation", m_fRotationInDegrees)
	SEOUL_PROPERTY_N("scaleX", m_fScaleX)
	SEOUL_PROPERTY_N("scaleY", m_fScaleY)
	SEOUL_PROPERTY_N("shearX", m_fShearX)
	SEOUL_PROPERTY_N("shearY", m_fShearY)
	SEOUL_PROPERTY_N("transform", m_eTransformMode)
	SEOUL_PROPERTY_N("skin", m_bSkinRequired)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::DataDefinition, TypeFlags::kDisableNew)
	SEOUL_ATTRIBUTE(CustomSerializeType, Animation2D::DataDefinition::CustomDeserializeType)

	SEOUL_PROPERTY_N("bones", m_vBones) // Bones must be first.
	SEOUL_PROPERTY_N("events", m_tEvents)
	SEOUL_PROPERTY_N("ik", m_vIk)
	SEOUL_PROPERTY_N("skeleton", m_MetaData)
	SEOUL_PROPERTY_N("skins", m_tSkins)
		SEOUL_ATTRIBUTE(CustomSerializeProperty, "DeserializeSkin")
	SEOUL_PROPERTY_N("slots", m_vSlots) // Slots must be after skins.
	SEOUL_PROPERTY_N("path", m_vPaths) // Paths must be after skins and slots.
	SEOUL_PROPERTY_N("transform", m_vTransforms)
	SEOUL_PROPERTY_N("animations", m_tClips) // Clips must be after at least events.

	SEOUL_METHOD(DeserializeSkin)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::EventDefinition)
	SEOUL_PROPERTY_N("float", m_f)
	SEOUL_PROPERTY_N("int", m_i)
	SEOUL_PROPERTY_N("string", m_s)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::IkDefinition)
	SEOUL_PROPERTY_N("bones", m_vBoneIds)
	SEOUL_PROPERTY_N("name", m_Id)
	SEOUL_PROPERTY_N("order", m_iOrder)
	SEOUL_PROPERTY_N("target", m_TargetId)
	SEOUL_PROPERTY_N("mix", m_fMix)
	SEOUL_PROPERTY_N("softness", m_fSoftness)
	SEOUL_PROPERTY_N("bendPositive", m_bBendPositive)
	SEOUL_PROPERTY_N("skin", m_bSkinRequired)
	SEOUL_PROPERTY_N("compress", m_bCompress)
	SEOUL_PROPERTY_N("stretch", m_bStretch)
	SEOUL_PROPERTY_N("uniform", m_bUniform)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::MetaData)
	SEOUL_PROPERTY_N("x", m_fPositionX)
	SEOUL_PROPERTY_N("y", m_fPositionY)
	SEOUL_PROPERTY_N("fps", m_fFPS)
	SEOUL_PROPERTY_N("height", m_fHeight)
	SEOUL_PROPERTY_N("width", m_fWidth)
SEOUL_END_TYPE()

SEOUL_BEGIN_ENUM(Animation2D::PathPositionMode)
	SEOUL_ENUM_N("percent", Animation2D::PathPositionMode::kPercent)
	SEOUL_ENUM_N("fixed", Animation2D::PathPositionMode::kFixed)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Animation2D::PathRotationMode)
	SEOUL_ENUM_N("tangent", Animation2D::PathRotationMode::kTangent)
	SEOUL_ENUM_N("chain", Animation2D::PathRotationMode::kChain)
	SEOUL_ENUM_N("chainScale", Animation2D::PathRotationMode::kChainScale)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(Animation2D::PathSpacingMode)
	SEOUL_ENUM_N("length", Animation2D::PathSpacingMode::kLength)
	SEOUL_ENUM_N("fixed", Animation2D::PathSpacingMode::kFixed)
	SEOUL_ENUM_N("percent", Animation2D::PathSpacingMode::kPercent)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(Animation2D::PathDefinition)
	SEOUL_PROPERTY_N("bones", m_vBoneIds)
	SEOUL_PROPERTY_N("name", m_Id)
	SEOUL_PROPERTY_N("position", m_fPosition)
	SEOUL_PROPERTY_N("translateMix", m_fPositionMix)
	SEOUL_PROPERTY_N("positionMode", m_ePositionMode)
	SEOUL_PROPERTY_N("rotation", m_fRotationInDegrees)
	SEOUL_PROPERTY_N("rotateMix", m_fRotationMix)
	SEOUL_PROPERTY_N("rotateMode", m_eRotationMode)
	SEOUL_PROPERTY_N("spacing", m_fSpacing)
	SEOUL_PROPERTY_N("spacingMode", m_eSpacingMode)
	SEOUL_PROPERTY_N("target", m_TargetId)
	SEOUL_PROPERTY_N("order", m_iOrder)
	SEOUL_PROPERTY_N("skin", m_bSkinRequired)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::SlotDataDefinition)
	SEOUL_PROPERTY_N("name", m_Id)
	SEOUL_PROPERTY_N("attachment", m_AttachmentId)
	SEOUL_PROPERTY_N("blend", m_eBlendMode)
	SEOUL_PROPERTY_N("color", m_Color)
	SEOUL_PROPERTY_N("bone", m_BoneId)
	SEOUL_PROPERTY_PAIR_N("dark", GetSecondaryColor, SetSecondaryColor)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Animation2D::TransformConstraintDefinition)
	SEOUL_PROPERTY_N("bones", m_vBoneIds)
	SEOUL_PROPERTY_N("name", m_Id)
	SEOUL_PROPERTY_N("x", m_fDeltaPositionX)
	SEOUL_PROPERTY_N("y", m_fDeltaPositionY)
	SEOUL_PROPERTY_N("rotation", m_fDeltaRotationInDegrees)
	SEOUL_PROPERTY_N("scaleX", m_fDeltaScaleX)
	SEOUL_PROPERTY_N("scaleY", m_fDeltaScaleY)
	SEOUL_PROPERTY_N("shearY", m_fDeltaShearY)
	SEOUL_PROPERTY_N("translateMix", m_fPositionMix)
	SEOUL_PROPERTY_N("rotateMix", m_fRotationMix)
	SEOUL_PROPERTY_N("scaleMix", m_fScaleMix)
	SEOUL_PROPERTY_N("shearMix", m_fShearMix)
	SEOUL_PROPERTY_N("target", m_TargetId)
	SEOUL_PROPERTY_N("order", m_iOrder)
	SEOUL_PROPERTY_N("skin", m_bSkinRequired)
	SEOUL_PROPERTY_N("local", m_bLocal)
	SEOUL_PROPERTY_N("relative", m_bRelative)
SEOUL_END_TYPE()

namespace Animation2D
{

DataDefinition::DataDefinition(FilePath filePath)
	: m_FilePath(filePath)
	, m_vBones()
	, m_tBones()
	, m_tClips()
	, m_vCurves()
	, m_tEvents()
	, m_vIk()
	, m_tIk()
	, m_MetaData()
	, m_vPaths()
	, m_tPaths()
	, m_vPoseTasks()
	, m_tSkins()
	, m_vSlots()
	, m_tSlots()
	, m_vTransforms()
	, m_tTransforms()
{
}

DataDefinition::~DataDefinition()
{
}

Bool DataDefinition::DeserializeSkin(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value)
{
	// Newer format - skin is an array instead of a table, due to additional constraints per skin.
	if (value.IsArray())
	{
		UInt32 uArrayCount = 0u;
		SEOUL_VERIFY(pDataStore->GetArrayCount(value, uArrayCount));

		Skins tSkins;
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			DataNode entry;
			SEOUL_VERIFY(pDataStore->GetValueFromArray(value, i, entry));

			HString name;
			DataNode nameNode;
			if (!pDataStore->GetValueFromTable(entry, kName, nameNode) ||
				!pDataStore->AsString(nameNode, name))
			{
				SEOUL_LOG_COOKING("Skin %u does not have a name", i);
				return false;
			}

			auto const e = tSkins.Insert(name, Attachments());
			if (!e.Second)
			{
				SEOUL_LOG_COOKING("Skin %u has duplicate name '%s'", i, name.CStr());
				return false;
			}

			auto& rAttachments = e.First->Second;
			DataNode attachmentsNode;
			if (pDataStore->GetValueFromTable(entry, kAttachments, attachmentsNode))
			{
				if (!Reflection::DeserializeObject(
					*pContext,
					*pDataStore,
					attachmentsNode,
					&rAttachments))
				{
					SEOUL_LOG_COOKING("Failed deserialization of attachments of skin %u -> '%s'", i, name.CStr());
					return false;
				}
			}
		}

		m_tSkins.Swap(tSkins);
		return true;
	}
	// Old format, direct deserialize.
	else
	{
		return Reflection::DeserializeObject(
			*pContext,
			*pDataStore,
			value,
			&m_tSkins);
	}
}

Bool DataDefinition::FinalizeBones()
{
	m_tBones.Clear();

	UInt32 const u = m_vBones.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto const& bone = m_vBones[i];
		SEOUL_VERIFY(m_tBones.Insert(bone.m_Id, (Int16)i).Second);
	}

	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& bone = m_vBones[i];
		if (!bone.m_ParentId.IsEmpty())
		{
			SEOUL_VERIFY(m_tBones.GetValue(bone.m_ParentId, bone.m_iParent));

			// Sanity check.
			if (bone.m_iParent >= (Int32)i)
			{
				return false;
			}
		}
	}

	return true;
}

Bool DataDefinition::FinalizeIk()
{
	UInt32 const u = m_vIk.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& ik = m_vIk[i];
		SEOUL_VERIFY(m_tIk.Insert(ik.m_Id, (Int16)i).Second);

		if (!m_tBones.GetValue(ik.m_TargetId, ik.m_iTarget))
		{
			return false;
		}

		UInt32 const uSize = ik.m_vBoneIds.GetSize();

		// Sanity check, this is assumed and required.
		if (uSize < 1u)
		{
			return false;
		}

		ik.m_viBones.Resize(uSize);
		for (UInt32 u = 0u; u < uSize; ++u)
		{
			if (!m_tBones.GetValue(ik.m_vBoneIds[u], ik.m_viBones[u]))
			{
				return false;
			}
		}
	}

	return true;
}

Bool DataDefinition::FinalizePaths()
{
	UInt32 const u = m_vPaths.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& path = m_vPaths[i];
		SEOUL_VERIFY(m_tPaths.Insert(path.m_Id, (Int16)i).Second);

		if (!m_tSlots.GetValue(path.m_TargetId, path.m_iTarget))
		{
			return false;
		}

		UInt32 const uSize = path.m_vBoneIds.GetSize();

		// Sanity check, this is assumed and required.
		if (uSize < 1u)
		{
			return false;
		}

		path.m_viBones.Resize(uSize);
		for (UInt32 u = 0u; u < uSize; ++u)
		{
			if (!m_tBones.GetValue(path.m_vBoneIds[u], path.m_viBones[u]))
			{
				return false;
			}
		}
	}

	return true;
}

namespace
{

struct PoseOrder SEOUL_SEALED
{
	PoseOrder(Int32 iType = 0, Int32 iOrder = 0, UInt32 uIndex = 0)
		: m_iType((Int8)iType)
		, m_iOrder((Int8)iOrder)
		, m_uIndex((UInt16)uIndex)
	{
		SEOUL_ASSERT(iType <= Int8Max);
		SEOUL_ASSERT(iOrder <= Int8Max);
		SEOUL_ASSERT(uIndex <= UInt16Max);
	}

	Bool operator<(const PoseOrder& b) const
	{
		// Descending, greater values first.
		return (m_iOrder < b.m_iOrder);
	}

	Int8 m_iType;
	Int8 m_iOrder;
	UInt16 m_uIndex;
}; // struct PoseOrder

} // namespace anonymous

typedef StackOrHeapArray<Bool, 64u, MemoryBudgets::Animation2D> BoneTracking;
static inline void EvalBone(
	const DataDefinition::Bones& vBones,
	Int16 iBone,
	BoneTracking& raB,
	DataDefinition::PoseTasks& rv)
{
	// Sanity and parent handling.
	if (iBone < 0)
	{
		return;
	}

	// Already evaluated, don't re-eval.
	auto& rb = raB[iBone];
	if (rb)
	{
		return;
	}

	// If the bone has a parent, make sure the parent is evaluated.
	auto const& bone = vBones[iBone];
	EvalBone(vBones, bone.m_iParent, raB, rv);

	// Bone is now evaluated.
	rb = true;

	// Don't insert an explicit entry for the root, this will
	// always be evaluated as a special case.
	if (0 != iBone)
	{
		rv.PushBack(PoseTask(PoseTaskType::kBone, iBone));
	}
}

static void EvalResetChildren(
	const DataDefinition::Bones& vBones,
	Int16 iParent,
	BoneTracking& raB)
{
	Int16 const iBones = (Int16)vBones.GetSize();
	for (Int16 i = iParent + 1; i < iBones; ++i)
	{
		if (vBones[i].m_iParent == iParent)
		{
			if (raB[i])
			{
				EvalResetChildren(vBones, i, raB);
			}

			raB[i] = false;
		}
	}
}

Bool DataDefinition::FinalizePoseTasks()
{
	UInt32 const uBones = m_vBones.GetSize();
	UInt32 const uIk = m_vIk.GetSize();
	UInt32 const uPaths = m_vPaths.GetSize();
	UInt32 const uTransforms = m_vTransforms.GetSize();

	m_vPoseTasks.Clear();
	m_vPoseTasks.Reserve(uBones + uIk);

	// Track whether bones have a task or not.
	BoneTracking ab(uBones);

	// Set the evaluation order.
	StackOrHeapArray<PoseOrder, 16u, MemoryBudgets::Animation2D> aOrder(uIk + uPaths + uTransforms);
	{
		auto uOut = 0u;
		for (UInt32 i = 0u; i < uIk; ++i) { aOrder[uOut++] = PoseOrder((Int32)PoseTaskType::kIk, m_vIk[i].m_iOrder, i); }
		for (UInt32 i = 0u; i < uPaths; ++i) { aOrder[uOut++] = PoseOrder((Int32)PoseTaskType::kPath, m_vPaths[i].m_iOrder, i); }
		for (UInt32 i = 0u; i < uTransforms; ++i) { aOrder[uOut++] = PoseOrder((Int32)PoseTaskType::kTransform, m_vTransforms[i].m_iOrder, i); }
	}
	QuickSort(aOrder.Begin(), aOrder.End());

	// Now process based on order.
	for (auto const& e : aOrder)
	{
		switch ((PoseTaskType)e.m_iType)
		{
		case PoseTaskType::kIk:
		{
			auto const& ik = m_vIk[e.m_uIndex];

			// Make sure the target and the first manipulated bone
			// are up-to-date prior to ik.
			EvalBone(m_vBones, ik.m_iTarget, ab, m_vPoseTasks);
			EvalBone(m_vBones, ik.m_viBones.Front(), ab, m_vPoseTasks);

			// Add the ik task.
			m_vPoseTasks.PushBack(PoseTask(PoseTaskType::kIk, (Int32)e.m_uIndex));

			// Allow any children of the first manipulated bone to be
			// re-evaluated.
			EvalResetChildren(m_vBones, ik.m_viBones.Front(), ab);

			// The child manipulated is now evaluated (controlled by the Ik).
			ab[ik.m_viBones[ik.m_viBones.GetSize() - 1]] = true;
		}
		break;

		case PoseTaskType::kPath:
		{
			auto const& path = m_vPaths[e.m_uIndex];

			// Make sure the target is evaluated prior to applying the path constraint.
			auto const& skins = GetSkins();

			// Acquire the set of possible path attachments.
			auto pSkin = skins.Find(kDefaultSkin /* The path attachments appear to always be on the default skin */);
			if (nullptr == pSkin)
			{
				SEOUL_WARN("%s: path constraint cannot be resolved, no default skin.", path.m_Id.CStr());
				return false;
			}

			auto pSets = pSkin->Find(path.m_TargetId);
			if (nullptr == pSets)
			{
				SEOUL_WARN("%s: path constraint cannot be resolved, attachments at target '%s'.", path.m_Id.CStr(), path.m_TargetId.CStr());
				return false;
			}

			auto const sortAttachment = [&](const SharedPtr<Attachment>& pAttachment)
			{
				// Must be a path attachment.
				if (pAttachment->GetType() != AttachmentType::kPath)
				{
					return false;
				}

				// See reference spine runtime - resolve all possible bone references of
				// the path attachment.
				SharedPtr<PathAttachment> pPathAttachment((PathAttachment*)pAttachment.GetPtr());
				auto const& bones = pPathAttachment->GetBoneCounts();
				auto const uBones = bones.GetSize();
				for (UInt32 i = 0u, n = uBones; i < n; )
				{
					auto nn = bones[i++];
					nn += i;
					while (i < nn)
					{
						EvalBone(m_vBones, bones[i++], ab, m_vPoseTasks);
					}
				}

				return true;
			};

			// Enumerate and process each attachment.
			for (auto const& pair : *pSets)
			{
				if (!sortAttachment(pair.Second))
				{
					return false;
				}
			}

			// Make sure the bones are up-to-date prior to applying the path constraint.
			for (auto const& bone : path.m_viBones)
			{
				EvalBone(m_vBones, bone, ab, m_vPoseTasks);
			}

			// Add the path task.
			m_vPoseTasks.PushBack(PoseTask(PoseTaskType::kPath, (Int32)e.m_uIndex));

			// Allow any children of the bones to be re-evaluated.
			for (auto const& bone : path.m_viBones)
			{
				EvalResetChildren(m_vBones, bone, ab);
			}

			// All manipulated bones are now considered evaluated.
			for (auto const& bone : path.m_viBones)
			{
				ab[bone] = true;
			}
		}
		break;

		case PoseTaskType::kTransform:
		{
			auto const& transform = m_vTransforms[e.m_uIndex];

			// Make sure the target and the bones are up-to-date
			// prior to applying the transform constraint.
			EvalBone(m_vBones, transform.m_iTarget, ab, m_vPoseTasks);
			if (transform.m_bLocal)
			{
				for (auto const& bone : transform.m_viBones)
				{
					// If the bone has a parent, make sure the parent is evaluated.
					EvalBone(m_vBones, m_vBones[bone].m_iParent, ab, m_vPoseTasks);
					EvalBone(m_vBones, bone, ab, m_vPoseTasks);
				}
			}
			else
			{
				for (auto const& bone : transform.m_viBones)
				{
					EvalBone(m_vBones, bone, ab, m_vPoseTasks);
				}
			}

			// Add the transform task.
			m_vPoseTasks.PushBack(PoseTask(PoseTaskType::kTransform, (Int32)e.m_uIndex));

			// Allow any children of the bones to be re-evaluated.
			for (auto const& bone : transform.m_viBones)
			{
				EvalResetChildren(m_vBones, bone, ab);
			}

			// All manipulated bones are now considered evaluated.
			for (auto const& bone : transform.m_viBones)
			{
				ab[bone] = true;
			}
		}
		break;

		default:
			SEOUL_FAIL("Out-of-sync enum, programmer error.");
			break;
		};
	}

	// Finally, add any bones not yet added. We skip the root, it is handled
	// specially.
	for (UInt32 i = 1u; i < uBones; ++i)
	{
		if (!ab[i])
		{
			m_vPoseTasks.PushBack(PoseTask(PoseTaskType::kBone, (Int16)i));
		}
	}

	return true;
}

Bool DataDefinition::FinalizeSkins()
{
	// Enumerate all skins, all slots, all attachments
	// and perform various post processing based on
	// attachment type.
	auto const iBeginSkin = m_tSkins.Begin();
	auto const iEndSkin = m_tSkins.End();
	for (auto iSkin = iBeginSkin; iEndSkin != iSkin; ++iSkin)
	{
		auto const iBeginSlot = iSkin->Second.Begin();
		auto const iEndSlot = iSkin->Second.End();
		for (auto iSlot = iBeginSlot; iEndSlot != iSlot; ++iSlot)
		{
			auto const iBeginAttachment = iSlot->Second.Begin();
			auto const iEndAttachment = iSlot->Second.End();
			for (auto iAttachment = iBeginAttachment; iEndAttachment != iAttachment; ++iAttachment)
			{
				auto p(iAttachment->Second);
				switch (p->GetType())
				{
				case AttachmentType::kLinkedMesh:
					{
						SharedPtr<LinkedMeshAttachment> pLinkedMesh((LinkedMeshAttachment*)p.GetPtr());

						// Resolve the attachment - lookup skin and then slot.
						auto const skinId(pLinkedMesh->GetSkinId().IsEmpty() ? kDefaultSkin : pLinkedMesh->GetSkinId());
						auto pSkin = m_tSkins.Find(skinId);
						if (nullptr == pSkin)
						{
							SEOUL_WARN("Linked mesh '%s' specifies skin '%s' which "
								"was not found.",
								iAttachment->First.CStr(),
								skinId.CStr());
							return false;
						}

						// Resolve slot in skin.
						auto pSlot = pSkin->Find(iSlot->First);
						if (nullptr == pSlot)
						{
							SEOUL_WARN("Linked mesh '%s' specifies skin '%s', but "
								"necessary slot '%s' as not found in that skin.",
								iAttachment->First.CStr(),
								skinId.CStr(),
								iSlot->First.CStr());
							return false;
						}

						// Attach the target mesh.
						SharedPtr<Attachment> pAttachment;
						if (!pSlot->GetValue(pLinkedMesh->m_ParentId, pAttachment))
						{
							SEOUL_WARN("Linked mesh '%s' specifies parent '%s' which "
								"cannot be found in skin '%s' and slot '%s'.",
								iAttachment->First.CStr(),
								pLinkedMesh->m_ParentId.CStr(),
								skinId.CStr(),
								iSlot->First.CStr());
							return false;
						}

						// Check type of attachment.
						if (pAttachment->GetType() != AttachmentType::kMesh)
						{
							SEOUL_WARN("Linked mesh '%s' specifies parent '%s' in "
								"skin '%s' and slot '%s', but the attachment is of unexpected type "
								"'%s'.",
								iAttachment->First.CStr(),
								pLinkedMesh->m_ParentId.CStr(),
								skinId.CStr(),
								iSlot->First.CStr(),
								EnumToString<AttachmentType>(pAttachment->GetType()));
							return false;
						}

						pLinkedMesh->m_pParent.Reset((MeshAttachment*)pAttachment.GetPtr());
					}
					break;
				case AttachmentType::kMesh:
					{
						// Generate the edge list for the mesh.
						SharedPtr<MeshAttachment> pMesh((MeshAttachment*)p.GetPtr());
						pMesh->ComputeEdges();
					}
					break;
				case AttachmentType::kPath:
					{
						SharedPtr<PathAttachment> pPath((PathAttachment*)p.GetPtr());
						pPath->m_Id = iAttachment->First;
						pPath->m_Slot = iSlot->First;
					}
					break;
				default:
					break;
				};
			}
		}
	}

	return true;
}

Bool DataDefinition::FinalizeSlots()
{
	UInt32 const u = m_vSlots.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& slot = m_vSlots[i];
		SEOUL_VERIFY(m_tSlots.Insert(slot.m_Id, (Int16)i).Second);

		if (!m_tBones.GetValue(slot.m_BoneId, slot.m_iBone))
		{
			return false;
		}
	}

	return true;
}

Bool DataDefinition::FinalizeTransforms()
{
	UInt32 const u = m_vTransforms.GetSize();
	for (UInt32 i = 0u; i < u; ++i)
	{
		auto& transform = m_vTransforms[i];
		SEOUL_VERIFY(m_tTransforms.Insert(transform.m_Id, (Int16)i).Second);

		if (!m_tBones.GetValue(transform.m_TargetId, transform.m_iTarget))
		{
			return false;
		}

		UInt32 const uSize = transform.m_vBoneIds.GetSize();

		// Sanity check, this is assumed and required.
		if (uSize < 1u)
		{
			return false;
		}

		transform.m_viBones.Resize(uSize);
		for (UInt32 u = 0u; u < uSize; ++u)
		{
			if (!m_tBones.GetValue(transform.m_vBoneIds[u], transform.m_viBones[u]))
			{
				return false;
			}
		}
	}

	return true;
}

Bool DataDefinition::Load(ReadWriteUtil& r)
{
	Bones vBones;
	Lookup tBones;
	Clips tClips;
	BezierCurves vCurves;
	Events tEvents;
	Ik vIk;
	Lookup tIk;
	MetaData metaData;
	Paths vPaths;
	Lookup tPaths;
	PoseTasks vPoseTasks;
	Skins tSkins;
	Slots vSlots;
	Lookup tSlots;
	Transforms vTransforms;
	Lookup tTransforms;

	auto bReturn = true;
	bReturn = bReturn  && r.Read(vBones);
	bReturn = bReturn  && r.Read(tBones);
	bReturn = bReturn  && r.Read(tClips);
	bReturn = bReturn  && r.Read(vCurves);
	bReturn = bReturn  && r.Read(tEvents);
	bReturn = bReturn  && r.Read(vIk);
	bReturn = bReturn  && r.Read(tIk);
	bReturn = bReturn  && r.Read(metaData);
	bReturn = bReturn  && r.Read(vPaths);
	bReturn = bReturn  && r.Read(tPaths);
	bReturn = bReturn  && r.Read(vPoseTasks);
	bReturn = bReturn  && r.Read(tSkins);
	bReturn = bReturn  && r.Read(vSlots);
	bReturn = bReturn  && r.Read(tSlots);
	bReturn = bReturn  && r.Read(vTransforms);
	bReturn = bReturn  && r.Read(tTransforms);

	// Done on failure.
	if (!bReturn)
	{
		return bReturn;
	}

	// Now resolve linked mesh parents - needs to be done
	// after most other structures have been loaded due to
	// dependency between skins.
	for (auto const& skinPair : tSkins)
	{
		for (auto const& slotPair : skinPair.Second)
		{
			for (auto const& attachmentPair : slotPair.Second)
			{
				// Only dealing with linked meshes.
				if (attachmentPair.Second->GetType() != AttachmentType::kLinkedMesh)
				{
					continue;
				}

				// Binary format would have been vetted, so we don't log here.
				// Just return false on errors for sanity, but we never expect
				// to hit these cases.
				auto pLinked((LinkedMeshAttachment*)attachmentPair.Second.GetPtr());

				// Use default skin if not explicitly defined.
				auto const skinId(pLinked->GetSkinId().IsEmpty() ? kDefaultSkin : pLinked->GetSkinId());

				// Resolve attachment parent.
				auto pSkin(tSkins.Find(skinId));
				if (nullptr == pSkin) { return false; }
				auto pSlot(pSkin->Find(slotPair.First));
				if (nullptr == pSlot) { return false; }
				auto ppParent(pSlot->Find(pLinked->GetParentId()));
				if (nullptr == ppParent) { return false; }
				if (!(*ppParent).IsValid()) { return false; }
				if ((*ppParent)->GetType() != AttachmentType::kMesh) { return false; }

				// Commit the parent.
				pLinked->m_pParent.Reset((MeshAttachment*)(ppParent->GetPtr()));
			}
		}
	}

	// Swap in results and return success.
	m_vBones.Swap(vBones);
	m_tBones.Swap(tBones);
	m_tClips.Swap(tClips);
	m_vCurves.Swap(vCurves);
	m_tEvents.Swap(tEvents);
	m_vIk.Swap(vIk);
	m_tIk.Swap(tIk);
	m_MetaData = metaData;
	m_vPaths.Swap(vPaths);
	m_tPaths.Swap(tPaths);
	m_vPoseTasks.Swap(vPoseTasks);
	m_tSkins.Swap(tSkins);
	m_vSlots.Swap(vSlots);
	m_tSlots.Swap(tSlots);
	m_vTransforms.Swap(vTransforms);
	m_tTransforms.Swap(tTransforms);
	return true;
}

Bool DataDefinition::Save(ReadWriteUtil& r) const
{
	auto bReturn = true;
	bReturn = bReturn  && r.Write(m_vBones);
	bReturn = bReturn  && r.Write(m_tBones);
	bReturn = bReturn  && r.Write(m_tClips);
	bReturn = bReturn  && r.Write(m_vCurves);
	bReturn = bReturn  && r.Write(m_tEvents);
	bReturn = bReturn  && r.Write(m_vIk);
	bReturn = bReturn  && r.Write(m_tIk);
	bReturn = bReturn  && r.Write(m_MetaData);
	bReturn = bReturn  && r.Write(m_vPaths);
	bReturn = bReturn  && r.Write(m_tPaths);
	bReturn = bReturn  && r.Write(m_vPoseTasks);
	bReturn = bReturn  && r.Write(m_tSkins);
	bReturn = bReturn  && r.Write(m_vSlots);
	bReturn = bReturn  && r.Write(m_tSlots);
	bReturn = bReturn  && r.Write(m_vTransforms);
	bReturn = bReturn  && r.Write(m_tTransforms);
	return bReturn;
}

/** Convenience utility, copies a buffer of Vector2D into a buffer of Floats. */
static inline void UnfoldCopy(const Vector<Vector2D, MemoryBudgets::Animation2D>& v, Vector<Float, MemoryBudgets::Animation2D>& rv)
{
	// Sanity check that Vector2D is tightly packed for
	// the copy below.
	SEOUL_STATIC_ASSERT(sizeof(Vector2D) == 2u * sizeof(Float));

	// Resolve output.
	rv.Resize(v.GetSize() * 2u);

	// Copy if not 0 size.
	if (!rv.IsEmpty())
	{
		memcpy(rv.Data(), v.Data(), rv.GetSizeInBytes());
	}
}

Bool DataDefinition::GetAttachment(
	HString skinId,
	HString slotId,
	HString attachmentId,
	SharedPtr<Attachment>& rOutAttachment) const
{
	auto const& skins = GetSkins();

	auto pSkin = skins.Find(skinId);
	if (nullptr == pSkin)
	{
		return false;
	}

	auto pSets = pSkin->Find(slotId);
	if (nullptr == pSets)
	{
		return false;
	}

	auto ppAttachment = pSets->Find(attachmentId);
	if (nullptr == ppAttachment)
	{
		return false;
	}

	rOutAttachment = *ppAttachment;
	return true;
}

Bool DataDefinition::CopyBaseVertices(
	HString skinId,
	HString slotId,
	HString attachmentId,
	Floats& rv) const
{
	SharedPtr<Attachment> pAttachment;
	if (!GetAttachment(skinId, slotId, attachmentId, pAttachment))
	{
		return false;
	}

	switch (pAttachment->GetType())
	{
	case AttachmentType::kLinkedMesh:
		{
			SharedPtr<LinkedMeshAttachment> p((LinkedMeshAttachment*)pAttachment.GetPtr());
			if (!p->GetParent().IsValid())
			{
				return false;
			}

			UnfoldCopy(p->GetParent()->GetVertices(), rv);
			return true;
		}
	case AttachmentType::kMesh:
		{
			SharedPtr<MeshAttachment> p((MeshAttachment*)pAttachment.GetPtr());

			UnfoldCopy(p->GetVertices(), rv);
			return true;
		}
	case AttachmentType::kPath:
		{
			SharedPtr<PathAttachment> p((PathAttachment*)pAttachment.GetPtr());
			rv = p->GetVertices();
			return true;
		}
	default:
		return false;
	};
}

Bool DataDefinition::operator==(const DataDefinition& b) const
{
	auto br = true;
	br = br && (m_FilePath == b.m_FilePath);
	br = br && (m_vBones == b.m_vBones);
	br = br && (m_tBones == b.m_tBones);
	br = br && (m_tClips == b.m_tClips);
	br = br && (m_vCurves == b.m_vCurves);
	br = br && (m_tEvents == b.m_tEvents);
	br = br && (m_vIk == b.m_vIk);
	br = br && (m_tIk == b.m_tIk);
	br = br && (m_MetaData == b.m_MetaData);
	br = br && (m_vPaths == b.m_vPaths);
	br = br && (m_tPaths == b.m_tPaths);
	br = br && (m_vPoseTasks == b.m_vPoseTasks);
	br = br && (m_tSkins == b.m_tSkins);
	br = br && (m_vSlots == b.m_vSlots);
	br = br && (m_tSlots == b.m_tSlots);
	br = br && (m_vTransforms == b.m_vTransforms);
	br = br && (m_tTransforms == b.m_tTransforms);
	return br;
}

Bool DataDefinition::CustomDeserializeType(
	Reflection::SerializeContext& rContext,
	const DataStore& dataStore,
	const DataNode& table,
	const Reflection::WeakAny& objectThis,
	Bool bSkipPostSerialize)
{
	// Set the bone's curve vector as the context user data
	// for the duration of the serialization, so key frames
	// have access to it.
	auto p = objectThis.Cast<Animation2D::DataDefinition*>();
	auto const old = rContext.GetUserData();
	rContext.SetUserData(p);

	// Perform the actual deserialization into this DataDefinition.
	Bool const bSuccess = Reflection::Type::TryDeserialize(
		rContext,
		dataStore,
		table,
		objectThis,
		bSkipPostSerialize,
		true);
	rContext.SetUserData(old);

	// On failure, fail immediately.
	if (!bSuccess)
	{
		return false;
	}

	// Perform post processing on success until now.
	if (!p->FinalizeBones()) { return false; }
	if (!p->FinalizeIk()) { return false; }
	if (!p->FinalizeSkins()) { return false; }
	if (!p->FinalizeSlots()) { return false; }
	if (!p->FinalizePaths()) { return false; } // Must come after FinalizeSlots().
	if (!p->FinalizeTransforms()) { return false; }
	if (!p->FinalizePoseTasks()) { return false; } // Must be last.

	return true;
}

} // namespace Animation2D

SEOUL_TYPE(Animation2DDataContentHandle)

SharedPtr<Animation2D::DataDefinition> Content::Traits<Animation2D::DataDefinition>::GetPlaceholder(FilePath filePath)
{
	return SharedPtr<Animation2D::DataDefinition>();
}

Bool Content::Traits<Animation2D::DataDefinition>::FileChange(FilePath filePath, const Animation2DDataContentHandle& hEntry)
{
	if (FileType::kAnimation2D == filePath.GetType())
	{
		Load(filePath, hEntry);
		return true;
	}

	return false;
}

void Content::Traits<Animation2D::DataDefinition>::Load(FilePath filePath, const Animation2DDataContentHandle& hEntry)
{
	Content::LoadManager::Get()->Queue(
		SharedPtr<Content::LoaderBase>(SEOUL_NEW(MemoryBudgets::Content) Animation2D::DataContentLoader(filePath, hEntry)));
}

Bool Content::Traits<Animation2D::DataDefinition>::PrepareDelete(FilePath filePath, Content::Entry<Animation2D::DataDefinition, KeyType>& entry)
{
	return true;
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
