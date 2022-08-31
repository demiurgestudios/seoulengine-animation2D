/**
 * \file Animation2DDataInstance.cpp
 * \brief Mutable container of per-frame instance state. Used to
 * capture an instance pose for query and rendering.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "AnimationEventInterface.h"
#include "Animation2DCache.h"
#include "Animation2DDataDefinition.h"
#include "Animation2DDataInstance.h"
#include "Matrix2D.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(Vector<Animation2D::SlotInstance, 15>)
SEOUL_BEGIN_TYPE(Animation2D::SlotInstance)
	SEOUL_PROPERTY_N("AttachmentId", m_AttachmentId)
	SEOUL_PROPERTY_N("Color", m_Color)
SEOUL_END_TYPE()

namespace Animation2D
{

/** Zero epsilon, see spine source code. */
static const Float32 kfPathEpsilon = 0.00001f;
static const Float32 kfPathEpsilonLoose = 0.001f;

static inline Bool FloatToBool(Bool bBase, Float f)
{
	return (((bBase ? 1.0f : 0.0f) + f) >= 0.5f);
}

BoneInstance& BoneInstance::Assign(const BoneDefinition& data)
{
	m_fPositionX = data.m_fPositionX;
	m_fPositionY = data.m_fPositionY;
	m_fRotationInDegrees = data.m_fRotationInDegrees;
	m_fScaleX = data.m_fScaleX;
	m_fScaleY = data.m_fScaleY;
	m_fShearX = data.m_fShearX;
	m_fShearY = data.m_fShearY;

	return *this;
}

IkInstance& IkInstance::Assign(const IkDefinition& data)
{
	m_bBendPositive = data.m_bBendPositive;
	m_fSoftness = data.m_fSoftness;
	m_fMix = data.m_fMix;
	m_bCompress = data.m_bCompress;
	m_bStretch = data.m_bStretch;
	m_bUniform = data.m_bUniform;

	return *this;
}

PathInstance& PathInstance::Assign(const PathDefinition& data)
{
	m_fPosition = data.m_fPosition;
	m_fPositionMix = data.m_fPositionMix;
	m_fRotationMix = data.m_fRotationMix;
	m_fSpacing = data.m_fSpacing;

	return *this;
}

SlotInstance& SlotInstance::Assign(const SlotDataDefinition& data)
{
	m_AttachmentId = data.m_AttachmentId;
	m_Color = data.m_Color;

	return *this;
}

TransformConstraintInstance& TransformConstraintInstance::Assign(const TransformConstraintDefinition& data)
{
	m_fPositionMix = data.m_fPositionMix;
	m_fRotationMix = data.m_fRotationMix;
	m_fScaleMix = data.m_fScaleMix;
	m_fShearMix = data.m_fShearMix;

	return *this;
}

DataInstance::DataInstance(const SharedPtr<DataDefinition const>& pData, const SharedPtr<Animation::EventInterface>& pEventInterface)
	: m_pCache(SEOUL_NEW(MemoryBudgets::Animation2D) Cache)
	, m_pData(pData)
	, m_pEventInterface(pEventInterface)
	, m_vBones()
	, m_tDeforms()
	, m_tDeformReferences()
	, m_vDrawOrder()
	, m_vIk()
	, m_vPaths()
	, m_vSkinningPalette()
	, m_vSlots()
	, m_vTransformConstraintStates()
{
	InternalConstruct();
}

DataInstance::~DataInstance()
{
	SafeDeleteTable(m_tDeforms);
}

DataInstance* DataInstance::Clone() const
{
	auto p = SEOUL_NEW(MemoryBudgets::Animation2D) DataInstance(m_pData, m_pEventInterface);
	p->m_vBones = m_vBones;
	{
		auto const iBegin = m_tDeforms.Begin();
		auto const iEnd = m_tDeforms.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			SEOUL_VERIFY(p->m_tDeforms.Insert(i->First, SEOUL_NEW(MemoryBudgets::Animation2D) DeformData(*(i->Second))).Second);
		}
	}
	p->m_vDrawOrder = m_vDrawOrder;
	p->m_vIk = m_vIk;
	p->m_vPaths = m_vPaths;
	p->m_vSkinningPalette = m_vSkinningPalette;
	p->m_vSlots = m_vSlots;
	p->m_vTransformConstraintStates = m_vTransformConstraintStates;
	return p;
}

/** Apply the current state of the animation cache to the instance state. This also resets the cache. */
void DataInstance::ApplyCache()
{
	auto const& data = *m_pData;
	auto const& vBones = data.GetBones();
	auto const& vIk = data.GetIk();
	auto const& vPaths = data.GetPaths();
	auto const& vSlotsData = data.GetSlots();
	auto const& vTransforms = data.GetTransforms();

	// Draw order.
	{
		if (m_pCache->m_vDrawOrder.IsEmpty())
		{
			SetDefaultDrawOrder(vSlotsData.GetSize(), m_vDrawOrder);
		}
		else
		{
			m_vDrawOrder = m_pCache->m_vDrawOrder;
		}
	}

	// Attachments.
	{
		if (!m_pCache->m_vAttachments.IsEmpty())
		{
			// Sort attachments - this should order them such that the highest alpha
			// attachment changes are last.
			QuickSort(m_pCache->m_vAttachments.Begin(), m_pCache->m_vAttachments.End());

			UInt32 const uSize = m_pCache->m_vAttachments.GetSize();

			// Now find the first attachment to apply - we apply all attachments that
			// have the highest alpha.
			UInt32 u = uSize - 1u;
			while (u > 0u)
			{
				if (m_pCache->m_vAttachments[u-1u].m_fAlpha < m_pCache->m_vAttachments[u].m_fAlpha)
				{
					break;
				}
				--u;
			}

			// Now apply the last set of attachments - we record any changes we make, since we
			// will "undo" all the other attachments that aren't part of this set.
			for (UInt32 i = u; i < uSize; ++i)
			{
				auto const& e = m_pCache->m_vAttachments[i];
				m_vSlots[e.m_iSlot].m_AttachmentId = e.m_AttachmentId;
				(void)m_pCache->m_SlotScratch.Insert(e.m_iSlot);
			}
		}

		// Now, undo (by applying the default attachment) all attachments that weren't
		// explicitly set for this frame.
		Int32 const iSlots = (Int32)m_vSlots.GetSize();
		for (Int32 iSlot = 0; iSlot < iSlots; ++iSlot)
		{
			// Skip if this was part of the highest weighted set.
			if (m_pCache->m_SlotScratch.HasKey(iSlot))
			{
				continue;
			}

			m_vSlots[iSlot].m_AttachmentId = vSlotsData[iSlot].m_AttachmentId;
		}
	}

	// Color.
	{
		Int32 const iSlots = (Int32)m_vSlots.GetSize();
		for (Int32 iSlot = 0; iSlot < iSlots; ++iSlot)
		{
			auto const& base = vSlotsData[iSlot];
			auto& r = m_vSlots[iSlot];

			auto p = m_pCache->m_tColor.Find(iSlot);
			if (nullptr == p)
			{
				r.m_Color = base.m_Color;
			}
			else
			{
				auto const& v = *p;

				r.m_Color.m_R = (UInt8)Min((Float)base.m_Color.m_R + v.X + 0.5f, 255.0f);
				r.m_Color.m_G = (UInt8)Min((Float)base.m_Color.m_G + v.Y + 0.5f, 255.0f);
				r.m_Color.m_B = (UInt8)Min((Float)base.m_Color.m_B + v.Z + 0.5f, 255.0f);
				r.m_Color.m_A = (UInt8)Min((Float)base.m_Color.m_A + v.W + 0.5f, 255.0f);
			}
		}
	}

	// Ik.
	{
		Int32 const iIks = (Int32)m_vIk.GetSize();
		for (Int32 iIk = 0; iIk < iIks; ++iIk)
		{
			auto const& base = vIk[iIk];
			auto& r = m_vIk[iIk];

			auto p = m_pCache->m_tIk.Find(iIk);
			if (nullptr == p)
			{
				r.m_fMix = base.m_fMix;
				r.m_fSoftness = base.m_fSoftness;
				r.m_bBendPositive = base.m_bBendPositive;
				r.m_bCompress = base.m_bCompress;
				r.m_bStretch = base.m_bStretch;
			}
			else
			{
				auto const& e = *p;
				r.m_fMix = (base.m_fMix + e.m_fMix);
				r.m_fSoftness = (base.m_fSoftness + e.m_fSoftness);
				r.m_bBendPositive = FloatToBool(base.m_bBendPositive, e.m_fBendPositive);
				r.m_bCompress = FloatToBool(base.m_bCompress, e.m_fCompress);
				r.m_bStretch = FloatToBool(base.m_bStretch, e.m_fStretch);
			}
		}
	}

	// Path.
	{
		Int32 const iPaths = (Int32)m_vPaths.GetSize();
		for (Int32 iPath = 0; iPath < iPaths; ++iPath)
		{
			auto const& base = vPaths[iPath];
			auto& r = m_vPaths[iPath];

			// Path mix.
			{
				auto p = m_pCache->m_tPathMix.Find(iPath);
				if (nullptr == p)
				{
					r.m_fPositionMix = base.m_fPositionMix;
					r.m_fRotationMix = base.m_fRotationMix;
				}
				else
				{
					auto const& v = *p;
					r.m_fPositionMix = base.m_fPositionMix + v.X;
					r.m_fRotationMix = base.m_fRotationMix + v.Y;
				}
			}

			// Path position.
			{
				auto p = m_pCache->m_tPathPosition.Find(iPath);
				if (nullptr == p)
				{
					r.m_fPosition = base.m_fPosition;
				}
				else
				{
					auto const f = *p;
					r.m_fPosition = base.m_fPosition + f;
				}
			}

			// Path spacing.
			{
				auto p = m_pCache->m_tPathSpacing.Find(iPath);
				if (nullptr == p)
				{
					r.m_fSpacing = base.m_fSpacing;
				}
				else
				{
					auto const f = *p;
					r.m_fSpacing = base.m_fSpacing + f;
				}
			}
		}
	}

	// Transforms
	{
		Int32 const iTransforms = (Int32)m_vTransformConstraintStates.GetSize();
		for (Int32 iTransform = 0; iTransform < iTransforms; ++iTransform)
		{
			auto const& base = vTransforms[iTransform];
			auto& r = m_vTransformConstraintStates[iTransform];

			auto p = m_pCache->m_tTransform.Find(iTransform);
			if (nullptr == p)
			{
				r.m_fPositionMix = base.m_fPositionMix;
				r.m_fRotationMix = base.m_fRotationMix;
				r.m_fScaleMix = base.m_fScaleMix;
				r.m_fShearMix = base.m_fShearMix;
			}
			else
			{
				auto const& v = *p;
				r.m_fPositionMix = base.m_fPositionMix + v.X;
				r.m_fRotationMix = base.m_fRotationMix + v.Y;
				r.m_fScaleMix = base.m_fScaleMix + v.Z;
				r.m_fShearMix = base.m_fShearMix + v.W;
			}
		}
	}

	// Bone transformation.
	{
		Int32 const iBones = (Int32)m_vBones.GetSize();
		for (Int32 iBone = 0; iBone < iBones; ++iBone)
		{
			auto const& base = vBones[iBone];
			auto& r = m_vBones[iBone];

			// Position
			{
				auto p = m_pCache->m_tPosition.Find(iBone);
				if (nullptr == p)
				{
					r.m_fPositionX = base.m_fPositionX;
					r.m_fPositionY = base.m_fPositionY;
				}
				else
				{
					auto const& v = *p;
					r.m_fPositionX = base.m_fPositionX + v.X;
					r.m_fPositionY = base.m_fPositionY + v.Y;
				}
			}

			// Rotation.
			{
				auto p = m_pCache->m_tRotation.Find(iBone);
				if (nullptr == p)
				{
					r.m_fRotationInDegrees = base.m_fRotationInDegrees;
				}
				else
				{
					r.m_fRotationInDegrees = ClampDegrees(base.m_fRotationInDegrees + *p);
				}
			}

			// Scale.
			{
				auto p = m_pCache->m_tScale.Find(iBone);
				if (nullptr == p)
				{
					r.m_fScaleX = base.m_fScaleX;
					r.m_fScaleY = base.m_fScaleY;
				}
				else
				{
					auto const& v = *p;
					Float const fBaseAlpha = (1.0f - Clamp(v.Z, 0.0f, 1.0f));
					r.m_fScaleX = (base.m_fScaleX * v.X) + (base.m_fScaleX * fBaseAlpha);
					r.m_fScaleY = (base.m_fScaleY * v.Y) + (base.m_fScaleY * fBaseAlpha);
				}
			}

			// Shear.
			{
				auto p = m_pCache->m_tShear.Find(iBone);
				if (nullptr == p)
				{
					r.m_fShearX = base.m_fShearX;
					r.m_fShearY = base.m_fShearY;
				}
				else
				{
					auto const& v = *p;
					r.m_fShearX = (base.m_fShearX + v.X);
					r.m_fShearY = (base.m_fShearY + v.Y);
				}
			}
		}
	}

	m_pCache->Clear();
}

/**
 * Prepare the skinning palette state of this instance for query and render.
 * Applies any animation changes made until now to the active skinning
 * palette.
 */
void DataInstance::PoseSkinningPalette()
{
	// Nothing to do if no bones.
	UInt32 const u = m_vSkinningPalette.GetSize();
	if (0u == u)
	{
		return;
	}

	// Root node updated first and specially. Assumed we
	// never see it again later. This is enforced by the
	// deserialization code of Animation2D::DataDefinition.
	m_vBones[0].ComputeWorldTransform(m_vSkinningPalette[0]);

	// Cache data.
	auto const& vTasks = m_pData->GetPoseTasks();

	// Now process the pose task list.
	for (auto const& task : vTasks)
	{
		switch ((PoseTaskType)task.m_iType)
		{
		case PoseTaskType::kBone:
			InternalPoseBone(task.m_iIndex);
			break;
		case PoseTaskType::kIk:
			InternalPoseIk(task.m_iIndex);
			break;
		case PoseTaskType::kPath:
			InternalPosePathConstraint(task.m_iIndex);
			break;
		case PoseTaskType::kTransform:
			InternalPoseTransformConstraint(task.m_iIndex);
			break;
		default:
			break;
		};
	}
}

void DataInstance::InternalConstruct()
{
	auto const& vBones = m_pData->GetBones();
	auto const& vIk = m_pData->GetIk();
	auto const& vPaths = m_pData->GetPaths();
	auto const& vSlots = m_pData->GetSlots();
	auto const& vTransforms = m_pData->GetTransforms();

	auto const uBones = vBones.GetSize();
	auto const uIk = vIk.GetSize();
	auto const uPaths = vPaths.GetSize();
	auto const uSlots = vSlots.GetSize();
	auto const uTransforms = vTransforms.GetSize();

	m_vBones.Resize(uBones);
	m_vDrawOrder.Resize(uSlots);
	m_vIk.Resize(uIk);
	m_vPaths.Resize(uPaths);
	m_vSkinningPalette.Resize(uBones, Matrix2x3::Identity());
	m_vSlots.Resize(uSlots);
	m_vTransformConstraintStates.Resize(uTransforms);

	for (auto i = 0u; i < uBones; ++i) { m_vBones[i].Assign(vBones[i]); }
	SetDefaultDrawOrder(uSlots, m_vDrawOrder);
	for (auto i = 0u; i < uIk; ++i) { m_vIk[i].Assign(vIk[i]); }
	for (auto i = 0u; i < uPaths; ++i) { m_vPaths[i].Assign(vPaths[i]); }
	for (auto i = 0u; i < uSlots; ++i) { m_vSlots[i].Assign(vSlots[i]); }
	for (auto i = 0u; i < uTransforms; ++i) { m_vTransformConstraintStates[i].Assign(vTransforms[i]); }

	PoseSkinningPalette();
}

SharedPtr<PathAttachment const> DataInstance::InternalGetPathAttachment(Int16 iTarget) const
{
	auto const& slotData = m_pData->GetSlots()[iTarget];
	auto const attachmentId = m_vSlots[iTarget].m_AttachmentId;

	if (attachmentId.IsEmpty())
	{
		return SharedPtr<PathAttachment const>();
	}

	auto pSkin = m_pData->GetSkins().Find(kDefaultSkin /* The path attachments appear to always be on the default skin */);
	if (nullptr == pSkin)
	{
		return SharedPtr<PathAttachment const>();
	}

	auto pSets = pSkin->Find(slotData.m_Id);
	if (nullptr == pSkin)
	{
		return SharedPtr<PathAttachment const>();
	}

	SharedPtr<Attachment> pAttachment;
	if (!pSets->GetValue(attachmentId, pAttachment))
	{
		return SharedPtr<PathAttachment const>();
	}

	if (pAttachment->GetType() != AttachmentType::kPath)
	{
		return SharedPtr<PathAttachment const>();
	}

	return SharedPtr<PathAttachment const>((PathAttachment const*)pAttachment.GetPtr());
}

void DataInstance::InternalPoseBone(Int16 iBone)
{
	auto const& state = m_vBones[iBone];
	InternalPoseBone(
		iBone,
		state.m_fPositionX,
		state.m_fPositionY,
		state.m_fRotationInDegrees,
		state.m_fScaleX,
		state.m_fScaleY,
		state.m_fShearX,
		state.m_fShearY);
}

void DataInstance::InternalPoseBone(
	Int16 iBone,
	Float fPositionX,
	Float fPositionY,
	Float fRotationInDegrees,
	Float fScaleX,
	Float fScaleY,
	Float fShearX,
	Float fShearY)
{
	auto& r = m_vSkinningPalette[iBone];
	auto const& data = m_pData->GetBones()[iBone];
	const Matrix2x3& mParent = m_vSkinningPalette[data.m_iParent];
	switch (data.m_eTransformMode)
	{
	case TransformMode::kNormal:
		BoneInstance::ComputeWorldTransform(fPositionX, fPositionY, fRotationInDegrees, fScaleX, fScaleY, fShearX, fShearY, r);
		r = mParent * r;
		break;
	case TransformMode::kOnlyTranslation:
		BoneInstance::ComputeWorldTransform(fPositionX, fPositionY, fRotationInDegrees, fScaleX, fScaleY, fShearX, fShearY, r);
		r = Matrix2x3::CreateFrom(
				Matrix2D(r),
				Matrix2x3::TransformPosition(mParent, r.GetTranslation()));
		break;
	case TransformMode::kNoRotationOrReflection: // fall-through
	case TransformMode::kNoScale: // fall-through
	case TransformMode::kNoScaleOrReflection:
		{
			Matrix2D mParent2x2;
			Matrix2D mBone2x2;
			switch (data.m_eTransformMode)
			{
			// Special handling when rotation and reflection
			// (but not scale) are disabled.
			//
			// See line 177 in Bone.cs in the spine-csharp.
			case TransformMode::kNoRotationOrReflection:
			{
				// Start with the full parent transform.
				mParent2x2 = mParent.GetUpper2x2();

				// Check for scaling and handle appropriately.
				auto fS = Vector2D(mParent2x2.M00, mParent2x2.M10).LengthSquared();
				auto fR = 0.0f;
				if (fS > 1e-4f)
				{
					fS = Abs(mParent2x2.Determinant()) / fS;
					mParent2x2.M01 = mParent2x2.M10 * fS;
					mParent2x2.M11 = mParent2x2.M00 * fS;
					fR = RadiansToDegrees(Atan2(mParent2x2.M10, mParent2x2.M00));
				}
				else
				{
					mParent2x2.M00 = 0.0f;
					mParent2x2.M10 = 0.0f;
					fR = 90.0f - RadiansToDegrees(Atan2(mParent2x2.M11, mParent2x2.M01));
				}

				// Negate M01 - this completes filling out the parent upper 2x2.
				mParent2x2.M01 = -mParent2x2.M01;

				// Now fill in the bone's 2x2.
				auto const fRX = DegreesToRadians(fRotationInDegrees + fShearX - fR);
				auto const fRY = DegreesToRadians(fRotationInDegrees + fShearY - fR + 90.0f);
				mBone2x2.M00 = Cos(fRX) * fScaleX;
				mBone2x2.M01 = Cos(fRY) * fScaleY;
				mBone2x2.M10 = Sin(fRX) * fScaleX;
				mBone2x2.M11 = Sin(fRY) * fScaleY;
			}
			break;

			// Special handling when parent scale
			// and or scale or reflection.
			//
			// See line 200 in Bone.cs in the spine-csharp.
			case TransformMode::kNoScale:
			case TransformMode::kNoScaleOrReflection:
			{
				// Fill out the parent transform.
				auto const fRotationRadians = DegreesToRadians(fRotationInDegrees);
				auto const fCos = Cos(fRotationRadians);
				auto const fSin = Sin(fRotationRadians);

				mParent2x2.M00 = mParent.M00 * fCos + mParent.M01 * fSin;
				mParent2x2.M10 = mParent.M10 * fCos + mParent.M11 * fSin;

				auto fS = Vector2D(mParent2x2.M00, mParent2x2.M10).Length();
				if (fS > 1e-4f)
				{
					fS = (1.0f / fS);
				}

				mParent2x2.M00 *= fS;
				mParent2x2.M10 *= fS;
				fS = Vector2D(mParent2x2.M00, mParent2x2.M10).Length();

				auto const fR = fPiOverTwo + Atan2(mParent2x2.M10, mParent2x2.M00);
				mParent2x2.M01 = Cos(fR) * fS;
				mParent2x2.M11 = Sin(fR) * fS;

				// Now fill in the bone transform - any rotation was
				// placed in the parent, so we just need to fill in adjust
				// scale and shearing.
				auto const fRX = DegreesToRadians(fShearX);
				auto const fRY = DegreesToRadians(fShearY + 90.0f);
				mBone2x2.M00 = Cos(fRX) * fScaleX;
				mBone2x2.M01 = Cos(fRY) * fScaleY;
				mBone2x2.M10 = Sin(fRX) * fScaleX;
				mBone2x2.M11 = Sin(fRY) * fScaleY;
			}
			break;
			default:
				SEOUL_FAIL("Invalid enum value.");
				break;
			};

			// Combine bone and parent transform into final - position
			// is always influenced by the full parent transform.
			r = Matrix2x3::CreateFrom(
				mParent2x2 * mBone2x2,
				Matrix2x3::TransformPosition(mParent, Vector2D(fPositionX, fPositionY)));

			// Final flipping.
			if (data.m_eTransformMode == TransformMode::kNoScale)
			{
				if (mParent.GetUpper2x2().Determinant() < 0.0f)
				{
					r.M01 = -r.M01;
					r.M11 = -r.M11;
				}
			}
		}
		break;
	default:
		SEOUL_FAIL("Invalid enum value.");
		break;
	};
}

void DataInstance::InternalPoseIk(Int16 iIk)
{
	auto const& data = m_pData->GetIk()[iIk];
	Int16 const iParent = data.m_viBones.Front();
	auto const& state = m_vIk[iIk];
	Int16 const iChild = data.m_viBones.Back();
	Int16 const iTarget = data.m_iTarget;
	auto& mT = m_vSkinningPalette[iTarget];

	switch (data.m_viBones.GetSize())
	{
	case 1:
		InternalPoseIk1(iParent, Vector2D(mT.TX, mT.TY), state.m_fMix, state.m_bCompress, state.m_bStretch, state.m_bUniform);
		break;
	case 2:
		InternalPoseIk2(iParent, iChild, Vector2D(mT.TX, mT.TY), state.m_fMix, (state.m_bBendPositive ? 1.0f : -1.0f), state.m_bStretch, state.m_fSoftness);
		break;
	default:
		break;
	};
}

/**
 * 1-bone ik constraint.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/IkConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseIk1(
	Int16 iParent,
	const Vector2D& vTarget,
	Float fAlpha,
	Bool bCompress,
	Bool bStretch,
	Bool bUniform)
{
	auto const& dataP = m_pData->GetBones()[iParent];
	auto const& stateP = m_vBones[iParent];
	auto const& mPP = m_vSkinningPalette[dataP.m_iParent];

	Float32 const fId = 1.0f / (mPP.M00 * mPP.M11 - mPP.M01 * mPP.M10);
	Float32 const fX = vTarget.X - mPP.TX;
	Float32 const fY = vTarget.Y - mPP.TY;
	Float32 fTX = (fX * mPP.M11 - fY * mPP.M01) * fId - stateP.m_fPositionX;
	Float32 fTY = (fY * mPP.M00 - fX * mPP.M10) * fId - stateP.m_fPositionY;

	Float32 fRotationIK = -stateP.m_fShearX - stateP.m_fRotationInDegrees;
	{
		auto const pa = mPP.M00;
		auto pb = mPP.M01;
		auto const pc = mPP.M10;
		auto pd = mPP.M11;
		switch (dataP.m_eTransformMode)
		{
		case TransformMode::kOnlyTranslation:
			fTX = vTarget.X - mPP.TX;
			fTY = vTarget.Y - mPP.TY;
			break;
		case TransformMode::kNoRotationOrReflection: {
			fRotationIK += RadiansToDegrees(Atan2(pc, pa));
			auto const ps = Abs(pa * pd - pb * pc) / (pa * pa + pc * pc);
			pb = -pc * ps;
			pd = pa * ps;

			auto const x = vTarget.X - mPP.TX;
			auto const y = vTarget.Y - mPP.TY;
			auto const d = pa * pd - pb * pc;

			fTX = (x * pd - y * pb) / d - stateP.m_fPositionX;
			fTY = (y * pa - x * pc) / d - stateP.m_fPositionY;
		} break;
		default: {
			auto const x = vTarget.X - mPP.TX;
			auto const y = vTarget.Y - mPP.TY;
			auto const d = pa * pd - pb * pc;

			fTX = (x * pd - y * pb) / d - stateP.m_fPositionX;
			fTY = (y * pa - x * pc) / d - stateP.m_fPositionY;
		} break;
		};
	}

	fRotationIK += RadiansToDegrees(Atan2(fTY, fTX));
	if (stateP.m_fScaleX < 0.0f)
	{
		fRotationIK += 180.0f;
	}
	fRotationIK = ClampDegrees(fRotationIK);

	auto fScaleX = stateP.m_fScaleX;
	auto fScaleY = stateP.m_fScaleY;
	if (bCompress || bStretch)
	{
		switch (dataP.m_eTransformMode)
		{
		case TransformMode::kNoScale:
			fTX = vTarget.X - mPP.TX;
			fTY = vTarget.Y - mPP.TY;
			break;
		case TransformMode::kNoScaleOrReflection:
			fTX = vTarget.X - mPP.TX;
			fTY = vTarget.Y - mPP.TY;
			break;
		default:
			break;
		};

		auto const b = (dataP.m_fLength * fScaleX);
		auto const dd = Sqrt(fTX * fTX + fTY * fTY);
		if ((bCompress && dd < b) || ((bStretch && dd > b) && b > 0.0001f))
		{
			auto const s = (((dd / b) - 1.0f) * fAlpha) + 1.0f;
			fScaleX *= s;
			if (bUniform)
			{
				fScaleY *= s;
			}
		}
	}

	InternalPoseBone(
		iParent,
		stateP.m_fPositionX,
		stateP.m_fPositionY,
		stateP.m_fRotationInDegrees + fRotationIK * fAlpha,
		fScaleX,
		fScaleY,
		stateP.m_fShearX,
		stateP.m_fShearY);
}

/**
 * 2-bone ik constraint.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/IkConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseIk2(
	Int16 iParent,
	Int16 iChild,
	const Vector2D& vTarget,
	Float fAlpha,
	Float fBendDirection,
	Bool bStretch,
	Float fSoftness)
{
	if (0.0f == fAlpha)
	{
		InternalPoseBone(iChild);
		return;
	}

	auto const& dataC = m_pData->GetBones()[iChild];
	auto const& dataP = m_pData->GetBones()[iParent];
	auto const& stateC = m_vBones[iChild];
	auto const& stateP = m_vBones[iParent];
	auto& mP = m_vSkinningPalette[iParent];

	Float32 px = stateP.m_fPositionX;
	Float32 py = stateP.m_fPositionY;
	Float32 psx = stateP.m_fScaleX;
	Float32 sx = psx;
	Float32 psy = stateP.m_fScaleY;
	Float32 csx = stateC.m_fScaleX;
	Int32 os1, os2, s2;
	if (psx < 0)
	{
		psx = -psx;
		os1 = 180;
		s2 = -1;
	}
	else
	{
		os1 = 0;
		s2 = 1;
	}

	if (psy < 0)
	{
		psy = -psy;
		s2 = -s2;
	}

	if (csx < 0)
	{
		csx = -csx;
		os2 = 180;
	}
	else
	{
		os2 = 0;
	}

	Float32 cx = stateC.m_fPositionX;
	Float32 cy;
	Float32 cwx;
	Float32 cwy;
	Float32 a = mP.M00;
	Float32 b = mP.M01;
	Float32 c = mP.M10;
	Float32 d = mP.M11;

	Bool const u = Abs(psx - psy) <= 0.0001f;
	if (!u)
	{
		cy = 0;
		cwx = a * cx + mP.TX;
		cwy = c * cx + mP.TY;
	}
	else
	{
		cy = stateC.m_fPositionY;
		cwx = a * cx + b * cy + mP.TX;
		cwy = c * cx + d * cy + mP.TY;
	}

	auto const& mPP = m_vSkinningPalette[dataP.m_iParent];
	a = mPP.M00;
	b = mPP.M01;
	c = mPP.M10;
	d = mPP.M11;
	Float32 const cross = (a * d - b * c);
	Float32 const id = IsZero(cross, 0.00001f) ? 0.0f : (1.0f / cross);
	Float32 x = cwx - mPP.TX;
	Float32 y = cwy - mPP.TY;
	Float32 dx = (x * d - y * b) * id - px;
	Float32 dy = (y * a - x * c) * id - py;
	Float32 l1 = Sqrt(dx * dx + dy * dy);
	Float32 l2 = dataC.m_fLength * csx;
	if (l1 < 0.0001f)
	{
		InternalPoseIk1(iParent, vTarget, fAlpha, false, bStretch, false);
		InternalPoseBone(
			iChild,
			cx,
			cy,
			0.0f,
			stateC.m_fScaleX,
			stateC.m_fScaleY,
			stateC.m_fShearX,
			stateC.m_fShearY);
		return;
	}

	x = vTarget.X - mPP.TX;
	y = vTarget.Y - mPP.TY;
	Float32 tx = (x * d - y * b) * id - px;
	Float32 ty = (y * a - x * c) * id - py;
	float dd = tx * tx + ty * ty;
	if (fSoftness != 0.0f) {
		fSoftness *= (psx * (csx + 1.0f)) / 2.0f;
		auto const td = Sqrt(dd);
		auto const sd = td - l1 - (l2 * psx) + fSoftness;
		if (sd > 0.0f)
		{
			auto p = Min(1.0f, sd / (fSoftness * 2.0f)) - 1.0f;
			p = (sd - fSoftness * (1 - p * p)) / td;
			tx -= p * tx;
			ty -= p * ty;
			dd = tx * tx + ty * ty;
		}
	}

	Float32 a1;
	Float32 a2;
	if (u)
	{
		l2 *= psx;
		Float32 cos = (dd - l1 * l1 - l2 * l2) / (2.0f * l1 * l2);
		if (cos < -1.0f)
		{
			cos = -1.0f;
		}
		else if (cos > 1.0f)
		{
			cos = 1.0f;
			if (bStretch)
			{
				sx *= (Sqrt(dd) / (l1 + l2) - 1.0f) * fAlpha + 1.0f;
			}
		}

		a2 = Acos(cos) * fBendDirection;
		a = l1 + l2 * cos;
		b = l2 * Sin(a2);
		a1 = Atan2(ty * a - tx * b, tx * a + ty * b);
	}
	else
	{
		a = psx * l2;
		b = psy * l2;
		Float32 aa = a * a;
		Float32 bb = b * b;
		Float32 ta = Atan2(ty, tx);
		c = bb * l1 * l1 + aa * dd - aa * bb;
		Float32 c1 = -2 * bb * l1;
		Float32 c2 = bb - aa;
		d = c1 * c1 - 4 * c2 * c;
		if (d >= 0)
		{
			Float32 q = Sqrt(d);
			if (c1 < 0)
			{
				q = -q;
			}
			q = -(c1 + q) / 2.0f;
			Float32 r0 = q / c2;
			Float32 r1 = c / q;
			Float32 r = Abs(r0) < Abs(r1) ? r0 : r1;
			if (r * r <= dd)
			{
				y = Sqrt(dd - r * r) * fBendDirection;
				a1 = ta - Atan2(y, r);
				a2 = Atan2(y / psy, (r - l1) / psx);
				goto break_outer;
			}
		}

		Float32 minAngle = fPi;
		Float32 minX = l1 - a;
		Float32 minDist = minX * minX;
		Float32 minY = 0.0f;
		Float32 maxAngle = 0.0f;
		Float32 maxX = l1 + a;
		Float32 maxDist = maxX * maxX;
		Float32 maxY = 0.0f;

		c = -a * l1 / (aa - bb);
		if (c >= -1.0f && c <= 1.0f)
		{
			c = Acos(c);
			x = a * Cos(c) + l1;
			y = b * Sin(c);
			d = x * x + y * y;
			if (d < minDist)
			{
				minAngle = c;
				minDist = d;
				minX = x;
				minY = y;
			}
			if (d > maxDist)
			{
				maxAngle = c;
				maxDist = d;
				maxX = x;
				maxY = y;
			}
		}

		if (dd <= (minDist + maxDist) / 2)
		{
			a1 = ta - Atan2(minY * fBendDirection, minX);
			a2 = minAngle * fBendDirection;
		}
		else
		{
			a1 = ta - Atan2(maxY * fBendDirection, maxX);
			a2 = maxAngle * fBendDirection;
		}
	}

break_outer:
	Float32 os = Atan2(cy, cx) * s2;
	Float32 rotation = stateP.m_fRotationInDegrees;
	a1 = ClampDegrees(RadiansToDegrees(a1 - os) + os1 - rotation);
	InternalPoseBone(
		iParent,
		px,
		py,
		rotation + a1 * fAlpha,
		sx,
		stateP.m_fScaleY,
		0,
		0);

	rotation = stateC.m_fRotationInDegrees;
	a2 = ClampDegrees((RadiansToDegrees(a2 + os) - stateC.m_fShearX) * s2 + os2 - rotation);
	InternalPoseBone(
		iChild,
		cx,
		cy,
		rotation + a2 * fAlpha,
		stateC.m_fScaleX,
		stateC.m_fScaleY,
		stateC.m_fShearX,
		stateC.m_fShearY);
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPosePathConstraint(Int16 iPath)
{
	auto& state = m_vPaths[iPath];
	Float32 const fPositionMix = state.m_fPositionMix;
	Float32 const fRotationMix = state.m_fRotationMix;
	Bool const bPosition = (fPositionMix > 0.0f);
	Bool const bRotation = (fRotationMix > 0.0f);
	if (!bPosition && !bRotation)
	{
		return;
	}

	auto const& data = m_pData->GetPaths()[iPath];
	auto const pPathAttachment = InternalGetPathAttachment(data.m_iTarget);
	if (!pPathAttachment.IsValid())
	{
		return;
	}

	Bool bPercentSpacing = (data.m_eSpacingMode == PathSpacingMode::kPercent);
	PathRotationMode const eRotationMode = data.m_eRotationMode;
	Bool const bTangents = (PathRotationMode::kTangent == eRotationMode);
	Bool const bScale = (PathRotationMode::kChainScale == eRotationMode);
	UInt32 const uBones = data.m_viBones.GetSize();
	UInt32 const uSpaces = (bTangents ? uBones : (uBones + 1u));

	state.m_vSpaces.Clear();
	state.m_vSpaces.Resize(uSpaces);
	auto& vSpaces = state.m_vSpaces;
	auto& vLengths = state.m_vLengths;
	Float32 const fSpacing = state.m_fSpacing;
	if (bScale || !bPercentSpacing)
	{
		if (bScale)
		{
			vLengths.Resize(uBones);
		}

		auto const bLengthSpacing = (data.m_eSpacingMode == PathSpacingMode::kLength);
		for (Int32 iBone = 0, n = (Int32)uSpaces - 1; iBone < n;)
		{
			auto const i = data.m_viBones[iBone];
			auto const& boneData = m_pData->GetBones()[i];
			auto& rPalette = m_vSkinningPalette[i];
			auto const fSetupLength = boneData.m_fLength;

			if (fSetupLength < kfPathEpsilon)
			{
				if (bScale)
				{
					vLengths[iBone] = 0.0f;
				}
				vSpaces[++iBone] = 0.0f;
			}
			else if (bPercentSpacing)
			{
				if (bScale)
				{
					auto const fX = (fSetupLength * rPalette.M00);
					auto const fY = (fSetupLength * rPalette.M10);
					auto const fLength = Sqrt(fX * fX + fY * fY);
					vLengths[iBone] = fLength;
				}
				vSpaces[++iBone] = fSpacing;
			}
			else
			{
				auto const fX = (fSetupLength * rPalette.M00);
				auto const fY = (fSetupLength * rPalette.M10);
				auto const fLength = Sqrt(fX * fX + fY * fY);
				if (bScale)
				{
					vLengths[iBone] = fLength;
				}
				vSpaces[++iBone] = (bLengthSpacing ? (fSetupLength + fSpacing) : fSpacing) * (fLength / fSetupLength);
			}
		}
	}
	else
	{
		for (UInt32 i = 1u; i < uSpaces; ++i)
		{
			vSpaces[i] = fSpacing;
		}
	}

	Float32 const* pfPoints = InternalPosePathConstraintPoints(
		iPath,
		pPathAttachment,
		uSpaces,
		bTangents,
		(PathPositionMode::kPercent == data.m_ePositionMode),
		bPercentSpacing);

	Vector2D vBone(pfPoints[0], pfPoints[1]);
	Float32 const fRotationInDegrees = data.m_fRotationInDegrees;
	Bool const bTip = (eRotationMode == PathRotationMode::kChain && fRotationInDegrees == 0);
	for (Int32 iBone = 0, iPoint = 3; iBone < (Int32)uBones; ++iBone, iPoint += 3)
	{
		auto const i = data.m_viBones[iBone];
		auto const& boneData = m_pData->GetBones()[i];
		auto& rPalette = m_vSkinningPalette[i];
		rPalette.TX += (vBone.X - rPalette.TX) * fPositionMix;
		rPalette.TY += (vBone.Y - rPalette.TY) * fPositionMix;
		Float32 const fX = pfPoints[iPoint];
		Float32 const fY = pfPoints[iPoint+1];
		Float32 const fDx = (fX - vBone.X);
		Float32 const fDy = (fY - vBone.Y);
		if (bScale)
		{
			Float32 const fLength = vLengths[iBone];
			if (fLength >= kfPathEpsilon)
			{
				Float32 s = (((Sqrt(fDx * fDx + fDy * fDy) / fLength) - 1.0f) * fRotationMix) + 1.0f;
				rPalette.M00 *= s;
				rPalette.M10 *= s;
			}
		}

		vBone.X = fX;
		vBone.Y = fY;

		if (bRotation)
		{
			Float32 const a = rPalette.M00;
			Float32 const b = rPalette.M01;
			Float32 const c = rPalette.M10;
			Float32 const d = rPalette.M11;
			Float32 fR;
			if (bTangents)
			{
				fR = pfPoints[iPoint-1];
			}
			else if (vSpaces[iBone + 1] < kfPathEpsilon)
			{
				fR = pfPoints[iPoint+2];
			}
			else
			{
				fR = Atan2(fDy, fDx);
			}
			fR -= Atan2(c, a) - DegreesToRadians(fRotationInDegrees);

			Float32 fCos;
			Float32 fSin;
			if (bTip)
			{
				fCos = Cos(fR);
				fSin = Sin(fR);
				Float32 const fLength = boneData.m_fLength;
				vBone.X += (fLength * (fCos * a - fSin * c) - fDx) * fRotationMix;
				vBone.Y += (fLength * (fSin * a + fCos * c) - fDy) * fRotationMix;
			}

			if (fR > fPi)
			{
				fR -= fTwoPi;
			}
			else if (fR < -fPi)
			{
				fR += fTwoPi;
			}

			fR *= fRotationMix;
			fCos = Cos(fR);
			fSin = Sin(fR);
			rPalette.M00 = (fCos * a - fSin * c);
			rPalette.M01 = (fCos * b - fSin * d);
			rPalette.M10 = (fSin * a + fCos * c);
			rPalette.M11 = (fSin * b + fCos * d);
		}
	}
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
static void AddBeforePosition(Float32 p, const Vector<Float, MemoryBudgets::Animation2D>& temp, Int i, Vector<Float, MemoryBudgets::Animation2D>& vOutput, Int o)
{
	Float32 x1 = temp[i];
	Float32 y1 = temp[i + 1];
	Float32 dx = temp[i + 2] - x1;
	Float32 dy = temp[i + 3] - y1;
	Float32 r = Atan2(dy, dx);
	vOutput[o] = x1 + p * Cos(r);
	vOutput[o + 1] = y1 + p * Sin(r);
	vOutput[o + 2] = r;
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
static void AddAfterPosition(Float32 p, const Vector<Float, MemoryBudgets::Animation2D>& temp, Int i, Vector<Float, MemoryBudgets::Animation2D>& vOutput, Int o)
{
	Float32 x1 = temp[i + 2];
	Float32 y1 = temp[i + 3];
	Float32 dx = x1 - temp[i];
	Float32 dy = y1 - temp[i + 1];
	Float32 r = Atan2(dy, dx);
	vOutput[o] = x1 + p * Cos(r);
	vOutput[o + 1] = y1 + p * Sin(r);
	vOutput[o + 2] = r;
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
static void AddCurvePosition(Float32 p, Float32 x1, Float32 y1, Float32 cx1, Float32 cy1, Float32 cx2, Float32 cy2, Float32 x2, Float32 y2, Vector<Float, MemoryBudgets::Animation2D>& vOutput, Int o, Bool tangents)
{
	if (p < kfPathEpsilon)
	{
		vOutput[o] = x1;
		vOutput[o + 1] = y1;
		vOutput[o + 2] = Atan2(cy1 - y1, cx1 - x1);
		return;
	}

	Float32 tt = p * p, ttt = tt * p;
	Float32 u = 1 - p;
	Float32 uu = u * u;
	Float32 uuu = uu * u;
	Float32 ut = u * p, ut3 = ut * 3;
	Float32 uut3 = u * ut3;
	Float32 utt3 = ut3 * p;
	Float32 x = x1 * uuu + cx1 * uut3 + cx2 * utt3 + x2 * ttt;
	Float32 y = y1 * uuu + cy1 * uut3 + cy2 * utt3 + y2 * ttt;

	vOutput[o] = x;
	vOutput[o + 1] = y;

	if (tangents)
	{
		if (p < kfPathEpsilonLoose)
		{
			vOutput[o + 2] = Atan2(cy1 - y1, cx1 - x1);
		}
		else
		{
			vOutput[o + 2] = Atan2(y - (y1 * uu + cy1 * ut * 2 + cy2 * tt), x - (x1 * uu + cx1 * ut * 2 + cx2 * tt));
		}
	}
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
static inline const PathAttachment::Vertices& ResolveVertices(
	const DataInstance& instance,
	const SharedPtr<PathAttachment const>& p)
{
	auto const slot = p->GetSlot();
	auto const id = p->GetId();

	DeformKey const key(kDefaultSkin /* TODO: Need to use the active skin. */, slot, id);

	CheckedPtr<DataInstance::DeformData> pData;
	(void)instance.GetDeforms().GetValue(key, pData);

	if (pData.IsValid())
	{
		return *pData;
	}
	else
	{
		return p->GetVertices();
	}
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
static void TransformToWorld(
	const DataInstance& instance,
	const DataInstance::SkinningPalette& vPalette,
	const Matrix2x3& m,
	const SharedPtr<PathAttachment const>& pPathAttachment,
	UInt32 uStart,
	UInt32 uCount,
	Vector<Float, MemoryBudgets::Animation2D>& rvOut,
	UInt32 uOffset)
{
	uCount += uOffset;

	auto const& vWeights = pPathAttachment->GetWeights();
	auto const& vVertices = ResolveVertices(instance, pPathAttachment);
	auto const& vB = pPathAttachment->GetBoneCounts();

	// Simpler case if no bones to manipulate.
	if (vB.IsEmpty())
	{
		for (UInt32 vv = uStart, w = uOffset; w < uCount; vv += 2u, w += 2u)
		{
			auto const vWorld(Matrix2x3::TransformPosition(
				m,
				Vector2D(vVertices[vv], vVertices[vv + 1u])));
			rvOut[w + 0u] = vWorld.X;
			rvOut[w + 1u] = vWorld.Y;
		}
		return;
	}

	// Complex case, weight with bones.
	UInt32 v = 0u;
	UInt32 uSkip = 0u;
	for (UInt32 i = 0u; i < uStart; i += 2u)
	{
		auto const uN = vB[v];
		v += (uN + 1u);
		uSkip += uN;
	}

	// Process and accumulate.
	for (UInt32 w = uOffset, uVertex = uSkip * 2u, uWeight = uSkip; w < uCount; w += 2u)
	{
		Vector2D vW(Vector2D::Zero());
		UInt32 uN = vB[v++];
		uN += v;

		for (; v < uN; v++, uVertex += 2u, ++uWeight)
		{
			auto const& m = vPalette[vB[v]];

			Vector2D const vVertex(vVertices[uVertex + 0u], vVertices[uVertex + 1u]);
			Float const fWeight = vWeights[uWeight];
			vW += Matrix2x3::TransformPosition(m, vVertex) * fWeight;
		}

		rvOut[w + 0u] = vW.X;
		rvOut[w + 1u] = vW.Y;
	}
}

/**
 * Part of path constraint application.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/PathConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
Float32 const* DataInstance::InternalPosePathConstraintPoints(
	Int16 iPath,
	const SharedPtr<PathAttachment const>& pPathAttachment,
	UInt32 uSpaces,
	Bool bTangents,
	Bool bPosition,
	Bool bSpacing)
{
	static const Int32 kNone = -1;
	static const Int32 kBefore = -2;
	static const Int32 kAfter = -3;

	auto const& data = m_pData->GetPaths()[iPath];
	auto& state = m_vPaths[iPath];
	auto const& mWorld = m_vSkinningPalette[m_pData->GetSlots()[data.m_iTarget].m_iBone];

	Float32 fPosition = state.m_fPosition;
	auto& vSpaces = state.m_vSpaces;

	state.m_vPositions.Clear();
	state.m_vPositions.Resize(uSpaces * 3u + 2u);
	auto& vOutput = state.m_vPositions;

	auto& vWorld = state.m_vWorld;
	vWorld.Clear();
	Bool const bClosed = (pPathAttachment->GetClosed());
	UInt32 uVertexComponents = (pPathAttachment->GetVertexCount());
	Int32 iCurveCount = uVertexComponents / 6;
	Int32 iPrevCurve = kNone;

	Float32 fPathLength = 0;
	if (!pPathAttachment->GetConstantSpeed())
	{
		auto const& vLengths = pPathAttachment->GetLengths();
		iCurveCount -= bClosed ? 1 : 2;
		fPathLength = vLengths[iCurveCount];
		if (bPosition)
		{
			fPosition *= fPathLength;
		}
		if (bSpacing)
		{
			for (UInt32 i = 1; i < uSpaces; ++i)
			{
				vSpaces[i] *= fPathLength;
			}
		}

		vWorld.Resize(8u);
		for (Int32 i = 0, o = 0, iCurve = 0; i < (Int32)uSpaces; i++, o += 3)
		{
			Float32 fSpace = vSpaces[i];
			fPosition += fSpace;
			Float32 p = fPosition;

			if (bClosed)
			{
				p = Fmod(p, fPathLength);
				if (p < 0)
				{
					p += fPathLength;
				}
				iCurve = 0;
			}
			else if (p < 0)
			{
				if (iPrevCurve != kBefore)
				{
					iPrevCurve = kBefore;
					TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, 2u, 4u, vWorld, 0u);
				}
				AddBeforePosition(p, vWorld, 0, vOutput, o);
				continue;
			}
			else if (p > fPathLength)
			{
				if (iPrevCurve != kAfter)
				{
					iPrevCurve = kAfter;
					TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, uVertexComponents - 6u, 4u, vWorld, 0u);
				}
				AddAfterPosition(p - fPathLength, vWorld, 0, vOutput, o);
				continue;
			}

			// Determine curve containing position.
			for (;; iCurve++)
			{
				Float32 fLength = vLengths[iCurve];
				if (p > fLength)
				{
					continue;
				}

				if (iCurve == 0)
				{
					p /= fLength;
				}
				else
				{
					Float32 prev = vLengths[iCurve - 1];
					p = (p - prev) / (fLength - prev);
				}
				break;
			}
			if (iCurve != iPrevCurve)
			{
				iPrevCurve = iCurve;
				if (bClosed && iCurve == iCurveCount)
				{
					TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, uVertexComponents - 4u, 4u, vWorld, 0u);
					TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, 0u, 4u, vWorld, 4u);
				}
				else
				{
					TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, iCurve * 6u + 2u, 8u, vWorld, 0u);
				}
			}

			AddCurvePosition(
				p,
				vWorld[0],
				vWorld[1],
				vWorld[2],
				vWorld[3],
				vWorld[4],
				vWorld[5],
				vWorld[6],
				vWorld[7],
				vOutput,
				o,
				bTangents || (i > 0 && fSpace < kfPathEpsilon));
		}

		return vOutput.Data();
	}

	// World vertices.
	if (bClosed)
	{
		uVertexComponents += 2;
		vWorld.Resize(uVertexComponents);
		TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, 2u, uVertexComponents - 4u, vWorld, 0u);
		TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, 0u, 2u, vWorld, uVertexComponents - 4u);
		vWorld[uVertexComponents - 2] = vWorld[0];
		vWorld[uVertexComponents - 1] = vWorld[1];
	}
	else
	{
		iCurveCount--;
		uVertexComponents -= 4;
		vWorld.Resize(uVertexComponents);
		TransformToWorld(*this, m_vSkinningPalette, mWorld, pPathAttachment, 2u, uVertexComponents, vWorld, 0u);
	}

	// Curve vLengths.
	auto& vCurves = state.m_vCurves;
	vCurves.Clear();
	vCurves.Resize(iCurveCount);
	fPathLength = 0;
	Float32 x1 = vWorld[0];
	Float32 y1 = vWorld[1];
	Float32 cx1 = 0;
	Float32 cy1 = 0;
	Float32 cx2 = 0;
	Float32 cy2 = 0;
	Float32 x2 = 0;
	Float32 y2 = 0;
	Float32 tmpx = 0;
	Float32 tmpy = 0;
	Float32 dddfx = 0;
	Float32 dddfy = 0;
	Float32 ddfx = 0;
	Float32 ddfy = 0;
	Float32 dfx = 0;
	Float32 dfy = 0;
	for (Int32 i = 0, w = 2; i < iCurveCount; i++, w += 6)
	{
		cx1 = vWorld[w];
		cy1 = vWorld[w + 1];
		cx2 = vWorld[w + 2];
		cy2 = vWorld[w + 3];
		x2 = vWorld[w + 4];
		y2 = vWorld[w + 5];
		tmpx = (x1 - cx1 * 2 + cx2) * 0.1875f;
		tmpy = (y1 - cy1 * 2 + cy2) * 0.1875f;
		dddfx = ((cx1 - cx2) * 3 - x1 + x2) * 0.09375f;
		dddfy = ((cy1 - cy2) * 3 - y1 + y2) * 0.09375f;
		ddfx = tmpx * 2 + dddfx;
		ddfy = tmpy * 2 + dddfy;
		dfx = (cx1 - x1) * 0.75f + tmpx + dddfx * 0.16666667f;
		dfy = (cy1 - y1) * 0.75f + tmpy + dddfy * 0.16666667f;
		fPathLength += Sqrt(dfx * dfx + dfy * dfy);
		dfx += ddfx;
		dfy += ddfy;
		ddfx += dddfx;
		ddfy += dddfy;
		fPathLength += Sqrt(dfx * dfx + dfy * dfy);
		dfx += ddfx;
		dfy += ddfy;
		fPathLength += Sqrt(dfx * dfx + dfy * dfy);
		dfx += ddfx + dddfx;
		dfy += ddfy + dddfy;
		fPathLength += Sqrt(dfx * dfx + dfy * dfy);
		vCurves[i] = fPathLength;
		x1 = x2;
		y1 = y2;
	}

	if (bPosition)
	{
		fPosition *= fPathLength;
	}
	else
	{
		fPosition *= (fPathLength / pPathAttachment->GetLengths()[iCurveCount - 1]);
	}

	if (bSpacing)
	{
		for (UInt32 i = 1u; i < uSpaces; i++)
		{
			vSpaces[i] *= fPathLength;
		}
	}

	auto& aSegments = state.m_aSegments;
	Float32 fCurveLength = 0;
	for (Int32 i = 0, o = 0, iCurve = 0, segment = 0; i < (Int32)uSpaces; i++, o += 3)
	{
		Float32 fSpace = vSpaces[i];
		fPosition += fSpace;
		Float32 p = fPosition;

		if (bClosed)
		{
			p = Fmod(p, fPathLength);
			if (p < 0)
			{
				p += fPathLength;
			}
			iCurve = 0;
		}
		else if (p < 0)
		{
			AddBeforePosition(p, vWorld, 0, vOutput, o);
			continue;
		}
		else if (p > fPathLength)
		{
			AddAfterPosition(p - fPathLength, vWorld, uVertexComponents - 4, vOutput, o);
			continue;
		}

		// Determine curve containing position.
		for (;; iCurve++)
		{
			Float32 fLength = vCurves[iCurve];
			if (p > fLength)
			{
				continue;
			}
			if (iCurve == 0)
			{
				p /= fLength;
			}
			else
			{
				Float32 prev = vCurves[iCurve - 1];
				p = (p - prev) / (fLength - prev);
			}
			break;
		}

		// Curve segment vLengths.
		if (iCurve != iPrevCurve)
		{
			iPrevCurve = iCurve;
			Int32 ii = iCurve * 6;
			x1 = vWorld[ii];
			y1 = vWorld[ii + 1];
			cx1 = vWorld[ii + 2];
			cy1 = vWorld[ii + 3];
			cx2 = vWorld[ii + 4];
			cy2 = vWorld[ii + 5];
			x2 = vWorld[ii + 6];
			y2 = vWorld[ii + 7];
			tmpx = (x1 - cx1 * 2 + cx2) * 0.03f;
			tmpy = (y1 - cy1 * 2 + cy2) * 0.03f;
			dddfx = ((cx1 - cx2) * 3 - x1 + x2) * 0.006f;
			dddfy = ((cy1 - cy2) * 3 - y1 + y2) * 0.006f;
			ddfx = tmpx * 2 + dddfx;
			ddfy = tmpy * 2 + dddfy;
			dfx = (cx1 - x1) * 0.3f + tmpx + dddfx * 0.16666667f;
			dfy = (cy1 - y1) * 0.3f + tmpy + dddfy * 0.16666667f;
			fCurveLength = Sqrt(dfx * dfx + dfy * dfy);
			aSegments[0] = fCurveLength;
			for (ii = 1; ii < 8; ii++)
			{
				dfx += ddfx;
				dfy += ddfy;
				ddfx += dddfx;
				ddfy += dddfy;
				fCurveLength += Sqrt(dfx * dfx + dfy * dfy);
				aSegments[ii] = fCurveLength;
			}
			dfx += ddfx;
			dfy += ddfy;
			fCurveLength += Sqrt(dfx * dfx + dfy * dfy);
			aSegments[8] = fCurveLength;
			dfx += ddfx + dddfx;
			dfy += ddfy + dddfy;
			fCurveLength += Sqrt(dfx * dfx + dfy * dfy);
			aSegments[9] = fCurveLength;
			segment = 0;
		}

		// Weight by segment fLength.
		p *= fCurveLength;
		for (;; segment++)
		{
			Float32 fLength = aSegments[segment];
			if (p > fLength)
			{
				continue;
			}

			if (segment == 0)
			{
				p /= fLength;
			}
			else
			{
				Float32 prev = aSegments[segment - 1];
				p = segment + (p - prev) / (fLength - prev);
			}

			break;
		}

		AddCurvePosition(
			p * 0.1f,
			x1,
			y1,
			cx1,
			cy1,
			cx2,
			cy2,
			x2,
			y2,
			vOutput,
			o,
			bTangents || (i > 0 && fSpace < kfPathEpsilon));
	}
	return vOutput.Data();
}

void DataInstance::InternalPoseTransformConstraint(Int16 iTransform)
{
	auto const& data = m_pData->GetTransforms()[iTransform];
	if (data.m_bLocal)
	{
		if (data.m_bRelative)
		{
			InternalPoseTransformConstraintRelativeLocal(iTransform);
		}
		else
		{
			InternalPoseTransformConstraintAbsoluteLocal(iTransform);
		}
	}
	else
	{
		if (data.m_bRelative)
		{
			InternalPoseTransformConstraintRelativeWorld(iTransform);
		}
		else
		{
			InternalPoseTransformConstraintAbsoluteWorld(iTransform);
		}
	}
}

/**
 * Transform constraint, absolute world space configuration.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/TransformConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseTransformConstraintAbsoluteWorld(Int16 iTransform)
{
	auto const& data = m_pData->GetTransforms()[iTransform];
	auto const& state = m_vTransformConstraintStates[iTransform];

	Float32 const fPositionMix = state.m_fPositionMix;
	Float32 const fRotationMix = state.m_fRotationMix;
	Float32 const fScaleMix = state.m_fScaleMix;
	Float32 const fShearMix = state.m_fShearMix;

	auto const& target = m_vSkinningPalette[data.m_iTarget];

	// Precompute offset factors for shear and rotation.
	auto fOffsetRotation = DegreesToRadians(data.m_fDeltaRotationInDegrees);
	auto fOffsetShear = DegreesToRadians(data.m_fDeltaShearY);

	// Invert shear and rotation if the target transform contains mirror.
	if (target.DeterminantUpper2x2() <= 0.0f)
	{
		fOffsetRotation = -fOffsetRotation;
		fOffsetShear = -fOffsetShear;
	}

	// Enumerate and apply.
	for (auto const& bone : data.m_viBones)
	{
		auto& m = m_vSkinningPalette[bone];

		if (fRotationMix > 0.0f)
		{
			Vector2D const vT0(target.GetColumn(0));
			Vector2D const vB0(m.GetColumn(0));

			Float32 const fRadians = fRotationMix *
				ClampRadians(Atan2(vT0.Y, vT0.X) - Atan2(vB0.Y, vB0.X) + fOffsetRotation);

			auto const initialRotationMatrix = m.GetUpper2x2();
			auto const rotationToApplyMatrix = Matrix2D::CreateRotation(fRadians);

			m.SetUpper2x2(rotationToApplyMatrix * initialRotationMatrix);
		}

		if (fPositionMix > 0.0f)
		{
			Vector2D const vOffset = (Matrix2x3::TransformPosition(
				target,
				Vector2D(data.m_fDeltaPositionX, data.m_fDeltaPositionY)) - m.GetTranslation()) * fPositionMix;
			m.SetTranslation(m.GetTranslation() + vOffset);
		}

		if (fScaleMix > 0.0f)
		{
			Float32 const fBoneScaleX = m.GetColumn(0).Length();
			Float32 const fTargetScaleX = target.GetColumn(0).Length();
			Float32 const fScaleX = (IsZero(fBoneScaleX, 1e-5f)
				? 0.0f
				: (fBoneScaleX + (fTargetScaleX - fBoneScaleX + data.m_fDeltaScaleX) * fScaleMix) / fBoneScaleX);

			Float32 const fBoneScaleY = m.GetColumn(1).Length();
			Float32 const fTargetScaleY = target.GetColumn(1).Length();
			Float32 const fScaleY = (IsZero(fBoneScaleY, 1e-5f)
				? 0.0f
				: (fBoneScaleY + (fTargetScaleY - fBoneScaleY + data.m_fDeltaScaleY) * fScaleMix) / fBoneScaleY);

			m.SetColumn(0, m.GetColumn(0) * fScaleX);
			m.SetColumn(1, m.GetColumn(1) * fScaleY);
		}

		if (fShearMix > 0.0f)
		{
			Vector2D const vT0(target.GetColumn(0));
			Vector2D const vT1(target.GetColumn(1));
			Vector2D const vB0(m.GetColumn(0));
			Vector2D const vB1(m.GetColumn(1));

			Float32 const fBY = Atan2(vB1.Y, vB1.X);
			Float32 const fR = ClampRadians(Atan2(vT1.Y, vT1.X) - Atan2(vT0.Y, vT0.X) - (fBY - Atan2(vB0.Y, vB0.X)));
			Float32 const fS = vB1.Length();
			Float32 const fFinalRotation = (fBY + (fR + fOffsetShear) * fShearMix);

			m.M01 = Cos(fFinalRotation) * fS;
			m.M11 = Sin(fFinalRotation) * fS;
		}
	}
}

/**
 * Transform constraint, relative world space configuration.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/TransformConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseTransformConstraintRelativeWorld(Int16 iTransform)
{
	auto const& data = m_pData->GetTransforms()[iTransform];
	auto const& state = m_vTransformConstraintStates[iTransform];

	Float32 const fPositionMix = state.m_fPositionMix;
	Float32 const fRotationMix = state.m_fRotationMix;
	Float32 const fScaleMix = state.m_fScaleMix;
	Float32 const fShearMix = state.m_fShearMix;

	auto const& target = m_vSkinningPalette[data.m_iTarget];

	// Precompute offset factors for shear and rotation.
	auto fOffsetRotation = DegreesToRadians(data.m_fDeltaRotationInDegrees);
	auto fOffsetShear = DegreesToRadians(data.m_fDeltaShearY);

	// Invert shear and rotation if the target transform contains mirror.
	if (target.DeterminantUpper2x2() <= 0.0f)
	{
		fOffsetRotation = -fOffsetRotation;
		fOffsetShear = -fOffsetShear;
	}

	// Enumerate and apply.
	for (auto const& bone : data.m_viBones)
	{
		auto& m = m_vSkinningPalette[bone];

		if (fRotationMix > 0.0f)
		{
			Vector2D const vT0(target.GetColumn(0));

			Float32 const fRadians = fRotationMix *
				ClampRadians(Atan2(vT0.Y, vT0.X) + fOffsetRotation);

			auto const initialRotationMatrix = m.GetUpper2x2();
			auto const rotationToApplyMatrix = Matrix2D::CreateRotation(fRadians);

			m.SetUpper2x2(rotationToApplyMatrix * initialRotationMatrix);
		}

		if (fPositionMix > 0.0f)
		{
			Vector2D const vOffset = (Matrix2x3::TransformPosition(
				target,
				Vector2D(data.m_fDeltaPositionX, data.m_fDeltaPositionY))) * fPositionMix;
			m.SetTranslation(m.GetTranslation() + vOffset);
		}

		if (fScaleMix > 0.0f)
		{
			Float32 const fTargetScaleX = target.GetColumn(0).Length();
			Float32 const fScaleX = 
				(1.0f + (fTargetScaleX - 1.0f + data.m_fDeltaScaleX) * fScaleMix);

			Float32 const fTargetScaleY = target.GetColumn(1).Length();
			Float32 const fScaleY = 
				(1.0f + (fTargetScaleY - 1.0f + data.m_fDeltaScaleY) * fScaleMix);

			m.SetColumn(0, m.GetColumn(0) * fScaleX);
			m.SetColumn(1, m.GetColumn(1) * fScaleY);
		}

		if (fShearMix > 0.0f)
		{
			Vector2D const vT0(target.GetColumn(0));
			Vector2D const vT1(target.GetColumn(1));
			Vector2D const vB1(m.GetColumn(1));

			Float32 const fBY = Atan2(vB1.Y, vB1.X);
			Float32 const fR = ClampRadians(Atan2(vT1.Y, vT1.X) - Atan2(vT0.Y, vT0.X));
			Float32 const fS = vB1.Length();
			Float32 const fFinalRotation = (fBY + (fR - fPiOverTwo + fOffsetShear) * fShearMix);

			m.M01 = Cos(fFinalRotation) * fS;
			m.M11 = Sin(fFinalRotation) * fS;
		}
	}
}

/**
 * Transform constraint, relative world space configuration.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/TransformConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseTransformConstraintAbsoluteLocal(Int16 iTransform)
{
	auto const& data = m_pData->GetTransforms()[iTransform];
	auto const& state = m_vTransformConstraintStates[iTransform];

	Float32 const fPositionMix = state.m_fPositionMix;
	Float32 const fRotationMix = state.m_fRotationMix;
	Float32 const fScaleMix = state.m_fScaleMix;
	Float32 const fShearMix = state.m_fShearMix;

	auto const& target = m_vBones[data.m_iTarget];

	// Enumerate and apply.
	for (auto const& iBone : data.m_viBones)
	{
		auto const& bone = m_vBones[iBone];

		auto fRotationInDegrees = bone.m_fRotationInDegrees;
		if (fRotationMix != 0.0f)
		{
			auto const fR = target.m_fRotationInDegrees - fRotationInDegrees + data.m_fDeltaRotationInDegrees;
			fRotationInDegrees += fR * fRotationMix;
		}

		auto fX = bone.m_fPositionX;
		auto fY = bone.m_fPositionY;
		if (fPositionMix != 0.0f)
		{
			fX += (target.m_fPositionX - fX + data.m_fDeltaPositionX) * fPositionMix;
			fY += (target.m_fPositionY - fY + data.m_fDeltaPositionY) * fPositionMix;
		}

		auto fScaleX = bone.m_fScaleX;
		auto fScaleY = bone.m_fScaleY;
		if (fScaleMix != 0.0f)
		{
			if (fScaleX != 0.0f)
			{
				fScaleX = (fScaleX + (target.m_fScaleX - fScaleX + data.m_fDeltaScaleX) * fScaleMix) / fScaleX;
			}

			if (fScaleY != 0.0f)
			{
				fScaleY = (fScaleY + (target.m_fScaleY - fScaleY + data.m_fDeltaScaleY) * fScaleMix) / fScaleY;
			}
		}

		auto fShearY = bone.m_fShearY;
		if (fShearMix != 0.0f)
		{
			auto const fR = target.m_fShearY - fShearY + data.m_fDeltaShearY;
			fShearY += fR * fShearMix;
		}

		bone.ComputeWorldTransform(
			fX,
			fY,
			fRotationInDegrees,
			fScaleX,
			fScaleY,
			bone.m_fShearX,
			fShearY,
			m_vSkinningPalette[iBone]);
	}
}

/**
 * Transform constraint, relative world space configuration.
 *
 * \sa https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/src/spine/TransformConstraint.c
 * Covered by the Spine Runtime license: https://github.com/EsotericSoftware/spine-runtimes/blob/master/spine-c/LICENSE
 */
void DataInstance::InternalPoseTransformConstraintRelativeLocal(Int16 iTransform)
{
	auto const& data = m_pData->GetTransforms()[iTransform];
	auto const& state = m_vTransformConstraintStates[iTransform];

	Float32 const fPositionMix = state.m_fPositionMix;
	Float32 const fRotationMix = state.m_fRotationMix;
	Float32 const fScaleMix = state.m_fScaleMix;
	Float32 const fShearMix = state.m_fShearMix;

	auto const& target = m_vBones[data.m_iTarget];

	// Enumerate and apply.
	for (auto const& iBone : data.m_viBones)
	{
		auto const& bone = m_vBones[iBone];

		auto fRotationInDegrees = bone.m_fRotationInDegrees;
		if (fRotationMix != 0.0f)
		{
			auto const fR = target.m_fRotationInDegrees + data.m_fDeltaRotationInDegrees;
			fRotationInDegrees += fR * fRotationMix;
		}

		auto fX = bone.m_fPositionX;
		auto fY = bone.m_fPositionY;
		if (fPositionMix != 0.0f)
		{
			fX += (target.m_fPositionX + data.m_fDeltaPositionX) * fPositionMix;
			fY += (target.m_fPositionY + data.m_fDeltaPositionY) * fPositionMix;
		}

		auto fScaleX = bone.m_fScaleX;
		auto fScaleY = bone.m_fScaleY;
		if (fScaleMix != 0.0f)
		{
			fScaleX *= ((target.m_fScaleX - 1.0f + data.m_fDeltaScaleX) * fScaleMix) + 1.0f;
			fScaleY *= ((target.m_fScaleY - 1.0f + data.m_fDeltaScaleY) * fScaleMix) + 1.0f;
		}

		auto fShearY = bone.m_fShearY;
		if (fShearMix != 0.0f)
		{
			auto const fR = target.m_fShearY + data.m_fDeltaShearY;
			fShearY += fR * fShearMix;
		}

		bone.ComputeWorldTransform(
			fX,
			fY,
			fRotationInDegrees,
			fScaleX,
			fScaleY,
			bone.m_fShearX,
			fShearY,
			m_vSkinningPalette[iBone]);
	}
}

} // namespace Animation2D

} // namespace Seoul

#endif // /#if SEOUL_WITH_ANIMATION_2D
