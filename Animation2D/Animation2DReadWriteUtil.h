/**
 * \file Animation2DBinaryUtil.h
 *
 * Copyright (c) 2018-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION_2D_BINARY_UTIL_H
#define ANIMATION_2D_BINARY_UTIL_H

#include "Animation2DAttachment.h"
#include "Animation2DClipDefinition.h"
#include "Animation2DDataDefinition.h"
#include "Path.h"
#include "StreamBuffer.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

static const UInt32 kuAnimation2DBinarySignature = 0x480129d0;
static const UInt32 kuAnimation2DBinaryVersion = 2u;

template <typename T> struct NeedsDirSeparatorFixup;
template <> struct NeedsDirSeparatorFixup<HString> { static const Bool Value = false; };
template <> struct NeedsDirSeparatorFixup<FilePathRelativeFilename> { static const Bool Value = true; };

class ReadWriteUtil SEOUL_SEALED
{
public:
	ReadWriteUtil(StreamBuffer& rBuffer, Platform ePlatform)
		: m_r(rBuffer)
		, m_ePlatform(ePlatform)
	{
	}

	ReadWriteUtil(StreamBuffer& rBuffer)
		: m_r(rBuffer)
		, m_ePlatform(PeekPlatform(rBuffer))
	{
	}

	Bool Read(Float32& r)
	{
		return m_r.Read(r);
	}

	Bool Read(BezierCurve& curve)
	{
		if (!m_r.Read(curve.Data(), curve.GetSizeInBytes())) { return false; }
		return true;
	}

	Bool Read(BoneDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_ParentId);
		bReturn = bReturn && Read(r.m_fLength);
		bReturn = bReturn && Read(r.m_fPositionX);
		bReturn = bReturn && Read(r.m_fPositionY);
		bReturn = bReturn && Read(r.m_fRotationInDegrees);
		bReturn = bReturn && Read(r.m_fScaleX);
		bReturn = bReturn && Read(r.m_fScaleY);
		bReturn = bReturn && Read(r.m_fShearX);
		bReturn = bReturn && Read(r.m_fShearY);
		bReturn = bReturn && m_r.Read(r.m_eTransformMode);
		bReturn = bReturn && Read(r.m_iParent);
		bReturn = bReturn && Read(r.m_bSkinRequired);
		return bReturn;
	}

	Bool Read(HString& r)
	{
		UInt16 u = 0;
		return (m_r.Read(u) && m_HStrings.Query(u, r));
	}

	Bool Read(Int16& r)
	{
		return m_r.Read(r);
	}

	Bool Read(Int32& r)
	{
		return m_r.Read(r);
	}

	Bool Read(MetaData& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && m_r.Read(r.m_fPositionX);
		bReturn = bReturn && m_r.Read(r.m_fPositionY);
		bReturn = bReturn && m_r.Read(r.m_fFPS);
		bReturn = bReturn && m_r.Read(r.m_fHeight);
		bReturn = bReturn && m_r.Read(r.m_fWidth);
		return bReturn;
	}

	template <typename K, typename V>
	Bool Read(HashTable<K, V, MemoryBudgets::Animation2D>& t)
	{
		UInt32 u = 0u;
		if (!m_r.Read(u)) { return false; }
		t.Reserve(u);
		t.Clear();
		for (UInt32 i = 0u; i < u; ++i)
		{
			K k;
			if (!Read(k)) { return false; }
			V v;
			if (!Read(v)) { return false; }

			if (!t.Insert(k, v).Second) { return false; }
		}

		return true;
	}

	// Special handling for the attachment table.
	Bool Read(HashTable<HString, SharedPtr<Attachment>, MemoryBudgets::Animation2D>& t)
	{
		UInt32 u = 0u;
		if (!m_r.Read(u)) { return false; }
		t.Reserve(u);
		t.Clear();
		for (UInt32 i = 0u; i < u; ++i)
		{
			HString k;
			if (!Read(k)) { return false; }
			SharedPtr<Attachment> v;
			if (!Read(v, t)) { return false; }

			if (!t.Insert(k, v).Second) { return false; }
		}

		return true;
	}

	template <typename T>
	Bool Read(Vector<T, MemoryBudgets::Animation2D>& r)
	{
		UInt32 u = 0u;
		if (!m_r.Read(u)) { return false; }
		r.Reserve(u);
		r.Clear();
		for (UInt32 i = 0u; i < u; ++i)
		{
			T v;
			if (!Read(v)) { return false; }

			r.PushBack(v);
		}

		return true;
	}

	Bool Read(SharedPtr<Clip>& r)
	{
		SharedPtr<Clip> p(SEOUL_NEW(MemoryBudgets::Animation2D) Clip);
		if (p->Load(*this))
		{
			r.Swap(p);
			return true;
		}

		return false;
	}

	Bool Read(IkDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vBoneIds);
		bReturn = bReturn && Read(r.m_viBones);
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_TargetId);
		bReturn = bReturn && Read(r.m_fMix);
		bReturn = bReturn && Read(r.m_fSoftness);
		bReturn = bReturn && Read(r.m_iOrder);
		bReturn = bReturn && Read(r.m_iTarget);
		bReturn = bReturn && Read(r.m_bBendPositive);
		bReturn = bReturn && Read(r.m_bSkinRequired);
		bReturn = bReturn && Read(r.m_bCompress);
		bReturn = bReturn && Read(r.m_bStretch);
		bReturn = bReturn && Read(r.m_bUniform);
		return bReturn;
	}

	Bool Read(PathDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vBoneIds);
		bReturn = bReturn && Read(r.m_viBones);
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_fPosition);
		bReturn = bReturn && Read(r.m_fPositionMix);
		bReturn = bReturn && m_r.Read(r.m_ePositionMode);
		bReturn = bReturn && Read(r.m_fRotationInDegrees);
		bReturn = bReturn && Read(r.m_fRotationMix);
		bReturn = bReturn && m_r.Read(r.m_eRotationMode);
		bReturn = bReturn && Read(r.m_fSpacing);
		bReturn = bReturn && m_r.Read(r.m_eSpacingMode);
		bReturn = bReturn && Read(r.m_TargetId);
		bReturn = bReturn && Read(r.m_iOrder);
		bReturn = bReturn && Read(r.m_iTarget);
		bReturn = bReturn && Read(r.m_bSkinRequired);
		return bReturn;
	}

	Bool Read(PoseTask& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_iIndex);
		bReturn = bReturn && Read(r.m_iType);
		return bReturn;
	}

	Bool Read(RGBA& r)
	{
		return m_r.Read(r.m_Value);
	}

	Bool Read(SlotDataDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_AttachmentId);
		bReturn = bReturn && m_r.Read(r.m_eBlendMode);
		bReturn = bReturn && Read(r.m_Color);
		bReturn = bReturn && Read(r.m_BoneId);
		bReturn = bReturn && Read(r.m_iBone);
		bReturn = bReturn && Read(r.m_SecondaryColor);
		bReturn = bReturn && Read(r.m_bHasSecondaryColor);
		return bReturn;
	}

	Bool Read(EventDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_f);
		bReturn = bReturn && Read(r.m_i);
		bReturn = bReturn && Read(r.m_s);
		return bReturn;
	}

	Bool Read(String& r)
	{
		return m_r.Read(r);
	}

	Bool Read(TransformConstraintDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vBoneIds);
		bReturn = bReturn && Read(r.m_viBones);
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_fDeltaPositionX);
		bReturn = bReturn && Read(r.m_fDeltaPositionY);
		bReturn = bReturn && Read(r.m_fDeltaRotationInDegrees);
		bReturn = bReturn && Read(r.m_fDeltaScaleX);
		bReturn = bReturn && Read(r.m_fDeltaScaleY);
		bReturn = bReturn && Read(r.m_fDeltaShearY);
		bReturn = bReturn && Read(r.m_fPositionMix);
		bReturn = bReturn && Read(r.m_fRotationMix);
		bReturn = bReturn && Read(r.m_fScaleMix);
		bReturn = bReturn && Read(r.m_fShearMix);
		bReturn = bReturn && Read(r.m_TargetId);
		bReturn = bReturn && Read(r.m_iOrder);
		bReturn = bReturn && Read(r.m_iTarget);
		bReturn = bReturn && Read(r.m_bSkinRequired);
		bReturn = bReturn && Read(r.m_bLocal);
		bReturn = bReturn && Read(r.m_bRelative);
		return bReturn;
	}

	Bool Read(FilePath& r)
	{
		GameDirectory eDirectory = GameDirectory::kUnknown;
		FileType eFileType = FileType::kUnknown;
		UInt16 uName = 0;
		FilePathRelativeFilename name;

		Bool bReturn = true;
		bReturn = bReturn && m_r.Read(eDirectory);
		bReturn = bReturn && m_r.Read(eFileType);
		bReturn = bReturn && m_r.Read(uName) && m_RelativePaths.Query(uName, name);
		r.SetDirectory(eDirectory);
		r.SetRelativeFilenameWithoutExtension(name);
		r.SetType(eFileType);
		return bReturn;
	}

	Bool Read(UInt32& u)
	{
		return m_r.Read(u);
	}

	Bool Read(Vector2D& r)
	{
		return m_r.Read(r);
	}

	Bool Read(Edge& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vAbsOneOverDiffT);
		bReturn = bReturn && Read(r.m_fSepSquared);
		bReturn = bReturn && Read(r.m_u);
		return bReturn;
	}

	Bool Read(MeshAttachmentBoneLink& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fWeight);
		bReturn = bReturn && Read(r.m_uIndex);
		return bReturn;
	}

	Bool Read(SharedPtr<Attachment>& r, const HashTable<HString, SharedPtr<Attachment>, MemoryBudgets::Animation2D>& t)
	{
		AttachmentType eType = AttachmentType::kBitmap;
		if (!m_r.Read(eType)) { return false; }

		SharedPtr<Attachment> p(Attachment::New(eType));
		if (!p.IsValid()) { return false; }

		if (!p->Load(*this))
		{
			return false;
		}

		r.Swap(p);
		return true;
	}

	Bool Read(Bool& r)
	{
		return m_r.Read(r);
	}

	Bool Read(UInt16& r)
	{
		return m_r.Read(r);
	}

	Bool Read(KeyFrameDrawOrder& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fTime);
		bReturn = bReturn && Read(r.m_vOffsets);
		return bReturn;
	}

	Bool Read(BoneKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vRotation);
		bReturn = bReturn && Read(r.m_vScale);
		bReturn = bReturn && Read(r.m_vShear);
		bReturn = bReturn && Read(r.m_vTranslation);
		return bReturn;
	}

	Bool Read(DrawOrderOffset& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_iOffset);
		bReturn = bReturn && Read(r.m_Slot);
		return bReturn;
	}

	Bool Read(PathKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vMix);
		bReturn = bReturn && Read(r.m_vPosition);
		bReturn = bReturn && Read(r.m_vSpacing);
		return bReturn;
	}

	Bool Read(SlotKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vAttachment);
		bReturn = bReturn && Read(r.m_vColor);
		bReturn = bReturn && Read(r.m_vTwoColor);
		return bReturn;
	}

	Bool ReadBaseKeyframe(BaseKeyFrame& r)
	{
		Bool bReturn = true;

		bReturn = bReturn && Read(r.m_fTime);
		UInt8 uType = 0;
		bReturn = bReturn && Read(uType);
		UInt32 uOffset = 0;
		bReturn = bReturn && Read(uOffset);

		r.m_uCurveType = uType;
		r.m_uCurveDataOffset = uOffset;

		return bReturn;
	}

	Bool Read(KeyFrameAttachment& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_fTime);
		return bReturn;
	}

	Bool Read(KeyFrameColor& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_Color);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameDeform& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_vVertices);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameEvent& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_f);
		bReturn = bReturn && Read(r.m_i);
		bReturn = bReturn && Read(r.m_s);
		bReturn = bReturn && Read(r.m_Id);
		bReturn = bReturn && Read(r.m_fTime);
		return bReturn;
	}

	Bool Read(KeyFrameIk& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_bStretch);
		bReturn = bReturn && Read(r.m_bCompress);
		bReturn = bReturn && Read(r.m_bBendPositive);
		bReturn = bReturn && Read(r.m_fSoftness);
		bReturn = bReturn && Read(r.m_fMix);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFramePathMix& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fPositionMix);
		bReturn = bReturn && Read(r.m_fRotationMix);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFramePathPosition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fPosition);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFramePathSpacing& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fSpacing);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrame2D& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fX);
		bReturn = bReturn && Read(r.m_fY);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameRotation& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fAngleInDegrees);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameScale& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fX);
		bReturn = bReturn && Read(r.m_fY);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameTransform& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_fPositionMix);
		bReturn = bReturn && Read(r.m_fRotationMix);
		bReturn = bReturn && Read(r.m_fScaleMix);
		bReturn = bReturn && Read(r.m_fShearMix);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(KeyFrameTwoColor& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Read(r.m_Color);
		bReturn = bReturn && Read(r.m_SecondaryColor);
		bReturn = bReturn && ReadBaseKeyframe(r);
		return bReturn;
	}

	Bool Read(UInt8& r)
	{
		return m_r.Read(r);
	}

	Bool Write(Float32 r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(Int32 r)
	{
		m_r.Write(r);
		return true;
	}

	/**
	 * Write an arbitrary POD value to the stream buffer at
	 * the current head position.
	 */
	template<typename T>
	Bool Write(T value)
	{
		m_r.Write(value);
		return true;
	}

	Bool Write(UInt8 r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(const BezierCurve& curve)
	{
		for (UInt32 i = 0u; i < curve.GetSize(); ++i)
		{
			if (!Write(curve[i])) { return false; }
		}
		return true;
	}

	Bool Write(const BoneDefinition& r)
	{
		Write(r.m_Id);
		Write(r.m_ParentId);
		m_r.Write(r.m_fLength);
		m_r.Write(r.m_fPositionX);
		m_r.Write(r.m_fPositionY);
		m_r.Write(r.m_fRotationInDegrees);
		m_r.Write(r.m_fScaleX);
		m_r.Write(r.m_fScaleY);
		m_r.Write(r.m_fShearX);
		m_r.Write(r.m_fShearY);
		m_r.Write(r.m_eTransformMode);
		m_r.Write(r.m_iParent);
		m_r.Write(r.m_bSkinRequired);
		return true;
	}

	Bool Write(HString r)
	{
		m_r.Write(m_HStrings.Cache(r));
		return true;
	}

	Bool Write(Int16 r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(const MetaData& v)
	{
		m_r.Write(v.m_fPositionX);
		m_r.Write(v.m_fPositionY);
		m_r.Write(v.m_fFPS);
		m_r.Write(v.m_fHeight);
		m_r.Write(v.m_fWidth);
		return true;
	}

	template <typename K, typename V>
	Bool Write(const HashTable<K, V, MemoryBudgets::Animation2D>& t)
	{
		m_r.Write(t.GetSize());
		for (auto const& e : t)
		{
			if (!Write(e.First)) { return false; }
			if (!Write(e.Second)) { return false; }
		}

		return true;
	}

	// Special handling for the attachment table.
	Bool Write(const HashTable<HString, SharedPtr<Attachment>, MemoryBudgets::Animation2D>& t)
	{
		// Need to emit entries so that linked meshes are after meshes.
		m_r.Write(t.GetSize());
		for (auto const& e : t)
		{
			// Skipped linked meshes for now.
			if (e.Second->GetType() == AttachmentType::kLinkedMesh)
			{
				continue;
			}

			if (!Write(e.First)) { return false; }
			if (!Write(e.Second)) { return false; }
		}
		for (auto const& e : t)
		{
			// Skipped linked meshes for now.
			if (e.Second->GetType() != AttachmentType::kLinkedMesh)
			{
				continue;
			}

			if (!Write(e.First)) { return false; }
			if (!Write(e.Second)) { return false; }
		}

		return true;
	}

	template <typename T>
	Bool Write(const Vector<T, MemoryBudgets::Animation2D>& v)
	{
		m_r.Write(v.GetSize());
		for (auto const& e : v)
		{
			if (!Write(e)) { return false; }
		}

		return true;
	}

	Bool Write(const IkDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vBoneIds);
		bReturn = bReturn && Write(r.m_viBones);
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_TargetId);
		bReturn = bReturn && Write(r.m_fMix);
		bReturn = bReturn && Write(r.m_fSoftness);
		bReturn = bReturn && Write(r.m_iOrder);
		bReturn = bReturn && Write(r.m_iTarget);
		bReturn = bReturn && Write(r.m_bBendPositive);
		bReturn = bReturn && Write(r.m_bSkinRequired);
		bReturn = bReturn && Write(r.m_bCompress);
		bReturn = bReturn && Write(r.m_bStretch);
		bReturn = bReturn && Write(r.m_bUniform);
		return bReturn;
	}

	Bool Write(const PathDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vBoneIds);
		bReturn = bReturn && Write(r.m_viBones);
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_fPosition);
		bReturn = bReturn && Write(r.m_fPositionMix);
		bReturn = bReturn && Write(r.m_ePositionMode);
		bReturn = bReturn && Write(r.m_fRotationInDegrees);
		bReturn = bReturn && Write(r.m_fRotationMix);
		bReturn = bReturn && Write(r.m_eRotationMode);
		bReturn = bReturn && Write(r.m_fSpacing);
		bReturn = bReturn && Write(r.m_eSpacingMode);
		bReturn = bReturn && Write(r.m_TargetId);
		bReturn = bReturn && Write(r.m_iOrder);
		bReturn = bReturn && Write(r.m_iTarget);
		bReturn = bReturn && Write(r.m_bSkinRequired);
		return bReturn;
	}

	Bool Write(const PoseTask& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_iIndex);
		bReturn = bReturn && Write(r.m_iType);
		return bReturn;
	}

	Bool Write(const RGBA& r)
	{
		return Write(r.m_Value);
	}

	Bool Write(const SharedPtr<Clip>& p)
	{
		return p->Save(*this);
	}

	Bool Write(const EventDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_f);
		bReturn = bReturn && Write(r.m_i);
		bReturn = bReturn && Write(r.m_s);
		return bReturn;
	}

	Bool Write(const SlotDataDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_AttachmentId);
		bReturn = bReturn && Write(r.m_eBlendMode);
		bReturn = bReturn && Write(r.m_Color);
		bReturn = bReturn && Write(r.m_BoneId);
		bReturn = bReturn && Write(r.m_iBone);
		bReturn = bReturn && Write(r.m_SecondaryColor);
		bReturn = bReturn && Write(r.m_bHasSecondaryColor);
		return bReturn;
	}

	Bool Write(const String& r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(const SharedPtr<Attachment>& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r->GetType());
		bReturn = bReturn && r->Save(*this);
		return bReturn;
	}

	Bool Write(const TransformConstraintDefinition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vBoneIds);
		bReturn = bReturn && Write(r.m_viBones);
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_fDeltaPositionX);
		bReturn = bReturn && Write(r.m_fDeltaPositionY);
		bReturn = bReturn && Write(r.m_fDeltaRotationInDegrees);
		bReturn = bReturn && Write(r.m_fDeltaScaleX);
		bReturn = bReturn && Write(r.m_fDeltaScaleY);
		bReturn = bReturn && Write(r.m_fDeltaShearY);
		bReturn = bReturn && Write(r.m_fPositionMix);
		bReturn = bReturn && Write(r.m_fRotationMix);
		bReturn = bReturn && Write(r.m_fScaleMix);
		bReturn = bReturn && Write(r.m_fShearMix);
		bReturn = bReturn && Write(r.m_TargetId);
		bReturn = bReturn && Write(r.m_iOrder);
		bReturn = bReturn && Write(r.m_iTarget);
		bReturn = bReturn && Write(r.m_bSkinRequired);
		bReturn = bReturn && Write(r.m_bLocal);
		bReturn = bReturn && Write(r.m_bRelative);
		return bReturn;
	}

	Bool Write(UInt32 u)
	{
		m_r.Write(u);
		return true;
	}

	Bool Write(FilePath filePath)
	{
		m_r.Write(filePath.GetDirectory());
		m_r.Write(filePath.GetType());
		m_r.Write(m_RelativePaths.Cache(filePath.GetRelativeFilenameWithoutExtension()));
		return true;
	}

	Bool Write(const Vector2D& r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(const Edge& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vAbsOneOverDiffT);
		bReturn = bReturn && Write(r.m_fSepSquared);
		bReturn = bReturn && Write(r.m_u);
		return bReturn;
	}

	Bool Write(const MeshAttachmentBoneLink& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fWeight);
		bReturn = bReturn && Write(r.m_uIndex);
		return bReturn;
	}

	Bool Write(Bool r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(UInt16 r)
	{
		m_r.Write(r);
		return true;
	}

	Bool Write(const KeyFrameDrawOrder& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fTime);
		bReturn = bReturn && Write(r.m_vOffsets);
		return bReturn;
	}

	Bool Write(const BoneKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vRotation);
		bReturn = bReturn && Write(r.m_vScale);
		bReturn = bReturn && Write(r.m_vShear);
		bReturn = bReturn && Write(r.m_vTranslation);
		return bReturn;
	}

	Bool Write(const DrawOrderOffset& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_iOffset);
		bReturn = bReturn && Write(r.m_Slot);
		return bReturn;
	}

	Bool Write(const PathKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vMix);
		bReturn = bReturn && Write(r.m_vPosition);
		bReturn = bReturn && Write(r.m_vSpacing);
		return bReturn;
	}

	Bool Write(const SlotKeyFrames& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vAttachment);
		bReturn = bReturn && Write(r.m_vColor);
		bReturn = bReturn && Write(r.m_vTwoColor);
		return bReturn;
	}

	Bool WriteBaseKeyframe(const BaseKeyFrame& r)
	{
		UInt8 uType = r.m_uCurveType;
		UInt32 uOffset = r.m_uCurveDataOffset;

		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fTime);
		bReturn = bReturn && Write(uType);
		bReturn = bReturn && Write(uOffset);
		return bReturn;
	}

	Bool Write(const KeyFrame2D& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fX);
		bReturn = bReturn && Write(r.m_fY);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameAttachment& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_fTime);
		return bReturn;
	}

	Bool Write(const KeyFrameColor& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_Color);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameDeform& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_vVertices);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameEvent& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_f);
		bReturn = bReturn && Write(r.m_i);
		bReturn = bReturn && Write(r.m_s);
		bReturn = bReturn && Write(r.m_Id);
		bReturn = bReturn && Write(r.m_fTime);
		return bReturn;
	}

	Bool Write(const KeyFrameIk& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_bStretch);
		bReturn = bReturn && Write(r.m_bCompress);
		bReturn = bReturn && Write(r.m_bBendPositive);
		bReturn = bReturn && Write(r.m_fSoftness);
		bReturn = bReturn && Write(r.m_fMix);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFramePathMix& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fPositionMix);
		bReturn = bReturn && Write(r.m_fRotationMix);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFramePathPosition& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fPosition);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFramePathSpacing& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fSpacing);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameRotation& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fAngleInDegrees);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameScale& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fX);
		bReturn = bReturn && Write(r.m_fY);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameTransform& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_fPositionMix);
		bReturn = bReturn && Write(r.m_fRotationMix);
		bReturn = bReturn && Write(r.m_fScaleMix);
		bReturn = bReturn && Write(r.m_fShearMix);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	Bool Write(const KeyFrameTwoColor& r)
	{
		Bool bReturn = true;
		bReturn = bReturn && Write(r.m_Color);
		bReturn = bReturn && Write(r.m_SecondaryColor);
		bReturn = bReturn && WriteBaseKeyframe(r);
		return bReturn;
	}

	static Platform PeekPlatform(StreamBuffer& r)
	{
		auto const uOffset = r.GetOffset();

		UInt32 uSignature = 0u;
		UInt32 uVersion = 0u;
		Platform ePlatform = keCurrentPlatform;
		Bool b = true;
		b = b && r.Read(uSignature);
		b = b && (kuAnimation2DBinarySignature == uSignature);
		b = b && r.Read(uVersion);
		b = b && (kuAnimation2DBinaryVersion == uVersion);
		b = b && r.Read(ePlatform);

		// In all cases, rewind.
		r.SeekToOffset(uOffset);

		if (b)
		{
			return ePlatform;
		}
		else
		{
			return keCurrentPlatform;
		}
	}

	Bool BeginRead()
	{
		UInt32 uSignature = 0u;
		UInt32 uVersion = 0u;
		Platform eUnusedPlatform = keCurrentPlatform;
		Bool bReturn = true;
		bReturn = bReturn && m_r.Read(uSignature);
		bReturn = bReturn && (kuAnimation2DBinarySignature == uSignature);
		bReturn = bReturn && m_r.Read(uVersion);
		bReturn = bReturn && (kuAnimation2DBinaryVersion == uVersion);
		bReturn = bReturn && m_r.Read(eUnusedPlatform);

		bReturn = bReturn && m_HStrings.Read(m_r, m_ePlatform);
		bReturn = bReturn && m_RelativePaths.Read(m_r, m_ePlatform);
		return bReturn;
	}

	Bool EndWrite()
	{
		StreamBuffer r;
		r.Write(kuAnimation2DBinarySignature);
		r.Write(kuAnimation2DBinaryVersion);
		r.Write(m_ePlatform);
		Bool bReturn = true;
		bReturn = bReturn && m_HStrings.Write(r, m_ePlatform, false);
		bReturn = bReturn && m_RelativePaths.Write(r, m_ePlatform, true);

		if (bReturn)
		{
			r.Write(m_r.GetBuffer(), m_r.GetTotalDataSizeInBytes());
			m_r.Swap(r);
		}

		return bReturn;
	}

private:
	SEOUL_DISABLE_COPY(ReadWriteUtil);

	template <typename T>
	class StringTable SEOUL_SEALED
	{
	public:
		StringTable()
		{
		}

		~StringTable()
		{
		}

		UInt16 Cache(T h)
		{
			UInt16 u = 0u;
			if (!m_tTable.GetValue(h, u))
			{
				u = (UInt16)m_vList.GetSize();
				m_vList.PushBack(h);
				SEOUL_VERIFY(m_tTable.Insert(h, u).Second);
			}

			return u;
		}

		Bool Read(StreamBuffer& r, Platform ePlatform)
		{
			Byte const chTarget = (Byte)Path::GetDirectorySeparatorChar(ePlatform);
			Byte const chCurrent = (Byte)Path::GetDirectorySeparatorChar(keCurrentPlatform);
			auto const bFixup = NeedsDirSeparatorFixup<T>::Value && (chCurrent != chTarget);

			List vList;
			Table tTable;
			UInt32 uCount = 0u;

			Bool bReturn = true;
			bReturn = bReturn && r.Read(uCount);

			vList.Reserve(uCount);
			for (UInt32 i = 0u; i < uCount; ++i)
			{
				UInt32 uLength = 0u;
				bReturn = bReturn && r.Read(uLength);

				T h;
				if (uLength > 0u)
				{
					if (r.GetOffset() + uLength > r.GetTotalDataSizeInBytes())
					{
						bReturn = false;
					}
					else if (uLength > 0u)
					{
						if (bFixup)
						{
							Vector<Byte, MemoryBudgets::Animation2D> v;
							v.Resize(uLength);
							r.Read(v.Data(), uLength);
							auto const iBegin = v.Begin();
							auto const iEnd = v.End();
							for (auto i = iBegin; iEnd != i; ++i)
							{
								if (*i == chTarget)
								{
									*i = chCurrent;
								}
							}

							h = T(v.Data(), uLength);
						}
						else
						{
							h = T(r.GetBuffer() + r.GetOffset(), uLength);
							r.SeekToOffset(r.GetOffset() + uLength);
						}
					}
				}

				vList.PushBack(h);
				if (!tTable.Insert(h, (UInt16)i).Second)
				{
					return false;
				}
			}

			if (bReturn)
			{
				m_vList.Swap(vList);
				m_tTable.Swap(tTable);
			}
			return bReturn;
		}

		Bool Query(UInt16 u, T& r) const
		{
			if (u < m_vList.GetSize())
			{
				r = m_vList[u];
				return true;
			}

			return false;
		}

		Bool Write(StreamBuffer& r, Platform ePlatform, Bool bPaths)
		{
			Byte const chTarget = (Byte)Path::GetDirectorySeparatorChar(ePlatform);
			Byte const chCurrent = (Byte)Path::GetDirectorySeparatorChar(keCurrentPlatform);
			auto const bFixup = bPaths && (chCurrent != chTarget);

			r.Write(m_vList.GetSize());
			for (auto const& name : m_vList)
			{
				auto const uLength = name.GetSizeInBytes();
				r.Write(uLength);
				if (uLength > 0u)
				{
					r.Write(name.CStr(), uLength);
				}

				// Fixup separators if necessary.
				if (bFixup)
				{
					auto const iBegin = r.GetBuffer() + r.GetOffset() - uLength;
					auto const iEnd = r.GetBuffer() + r.GetOffset();
					for (auto i = iBegin; iEnd != i; ++i)
					{
						if (*i == chCurrent)
						{
							*i = chTarget;
						}
					}
				}
			}

			return true;
		}

	private:
		SEOUL_DISABLE_COPY(StringTable);

		typedef Vector<T, MemoryBudgets::Animation2D> List;
		typedef HashTable<T, UInt16, MemoryBudgets::Animation2D> Table;

		List m_vList;
		Table m_tTable;
	}; // class StringTable

	StreamBuffer& m_r;
	Platform const m_ePlatform;
	StringTable<HString> m_HStrings;
	StringTable<FilePathRelativeFilename> m_RelativePaths;
}; // class ReadWriteUtil

inline static void Obfuscate(
	Byte* pData,
	UInt32 zDataSizeInBytes,
	FilePath filePath)
{
	// Get the base filename.
	String const s(Path::GetFileNameWithoutExtension(filePath.GetRelativeFilenameWithoutExtension().ToString()));

	UInt32 uXorKey = 0x90B43928;
	for (UInt32 i = 0u; i < s.GetSize(); ++i)
	{
		uXorKey = uXorKey * 33 + tolower(s[i]);
	}

	for (UInt32 i = 0u; i < zDataSizeInBytes; ++i)
	{
		// Mix in the file offset
		pData[i] ^= (Byte)((uXorKey >> ((i % 4) << 3)) + (i / 4) * 101);
	}
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
