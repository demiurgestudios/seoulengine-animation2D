/**
 * \file Animation2DCache.h
 * \brief A cache is used to accumulate animation data for a frame,
 * which is then applied to compute the new skeleton pose at
 * the end of animation updating.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_CACHE_H
#define ANIMATION2D_CACHE_H

#include "HashSet.h"
#include "HashTable.h"
#include "Vector.h"
#include "Vector2D.h"
#include "Vector3D.h"
#include "Vector4D.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

struct Cache SEOUL_SEALED
{
	struct IkEntry SEOUL_SEALED
	{
		Float32 m_fMix{};
		Float32 m_fSoftness{};
		Float32 m_fBendPositive{};
		Float32 m_fCompress{};
		Float32 m_fStretch{};

		IkEntry& operator+=(const IkEntry& b)
		{
			m_fMix += b.m_fMix;
			m_fSoftness += b.m_fSoftness;
			m_fBendPositive += b.m_fBendPositive;
			m_fCompress += b.m_fCompress;
			m_fStretch += b.m_fStretch;

			return *this;
		}
	};

	struct SlotAttachmentEntry SEOUL_SEALED
	{
		SlotAttachmentEntry(Int16 iSlot = 0, HString attachmentId = HString(), Float fAlpha = 0.0f)
			: m_fAlpha(fAlpha)
			, m_AttachmentId(attachmentId)
			, m_iSlot(iSlot)
		{
		}

		Bool operator<(const SlotAttachmentEntry& b) const
		{
			return (m_fAlpha == b.m_fAlpha
				? (m_iSlot < b.m_iSlot)
				: (m_fAlpha < b.m_fAlpha));
		}

		Float m_fAlpha;
		HString m_AttachmentId;
		Int16 m_iSlot;
	}; // struct SlotAttachmentEntry

	struct TwoColorEntry SEOUL_SEALED
	{
		TwoColorEntry(
			Float fLightR,
			Float fLightG,
			Float fLightB,
			Float fLightA,
			Float fDarkR,
			Float fDarkG,
			Float fDarkB)
			: m_vLight(fLightR, fLightG, fLightB, fLightA)
			, m_vDark(fDarkR, fDarkG, fDarkB)
		{
		}

		TwoColorEntry& operator+=(const TwoColorEntry& b)
		{
			m_vLight += b.m_vLight;
			m_vDark += b.m_vDark;

			return *this;
		}

		Vector4D m_vLight;
		Vector3D m_vDark;
	};

	typedef HashTable<Int64, IkEntry, MemoryBudgets::Animation2D> CacheIk;
	typedef HashSet<Int16, MemoryBudgets::Animation2D> CacheSlot;
	typedef HashTable<Int16, Float, MemoryBudgets::Animation2D> Cache1D;
	typedef HashTable<Int16, Vector2D, MemoryBudgets::Animation2D> Cache2D;
	typedef HashTable<Int16, Vector3D, MemoryBudgets::Animation2D> Cache3D;
	typedef HashTable<Int16, Vector4D, MemoryBudgets::Animation2D> Cache4D;
	typedef Vector<SlotAttachmentEntry, MemoryBudgets::Animation2D> CacheSlotAttachments;
	typedef HashTable<Int64, TwoColorEntry, MemoryBudgets::Animation2D> CacheTwoColor;
	typedef Vector<Int16, MemoryBudgets::Animation2D> DrawOrder;

	template <typename T, typename U>
	static void AccumCommon(T& r, Int16 i, const U& v)
	{
		auto const e = r.Insert(i, v);
		if (!e.Second)
		{
			e.First->Second += v;
		}
	}

	void AccumIk(Int16 i, const IkEntry& e) { AccumCommon(m_tIk, i, e); }
	void AccumPathMix(Int16 i, const Vector2D& v) { AccumCommon(m_tPathMix, i, v); }
	void AccumPathPosition(Int16 i, Float f) { AccumCommon(m_tPathPosition, i, f); }
	void AccumPathSpacing(Int16 i, Float f) { AccumCommon(m_tPathSpacing, i, f); }
	void AccumPosition(Int16 i, const Vector2D& v) { AccumCommon(m_tPosition, i, v); }
	void AccumRotation(Int16 i, Float f) { AccumCommon(m_tRotation, i, f); }
	void AccumScale(Int16 i, const Vector2D& v, Float fAlpha) { AccumCommon(m_tScale, i, Vector3D(v, fAlpha)); }
	void AccumShear(Int16 i, const Vector2D& v) { AccumCommon(m_tShear, i, v); }

	void AccumSlotAttachment(Int16 iSlot, HString attachmentId, Float fAlpha)
	{
		m_vAttachments.PushBack(SlotAttachmentEntry(iSlot, attachmentId, fAlpha));
	}

	void AccumSlotColor(Int16 i, const Vector4D& v) { AccumCommon(m_tColor, i, v); }
	void AccumSlotTwoColor(Int16 i, const TwoColorEntry& e) { AccumCommon(m_tTwoColor, i, e); }
	void AccumTransform(Int16 i, const Vector4D& v) { AccumCommon(m_tTransform, i, v); }

	void Clear()
	{
		m_vAttachments.Clear();
		m_tColor.Clear();
		m_tTwoColor.Clear();
		m_vDrawOrder.Clear();
		m_tIk.Clear();
		m_tPathMix.Clear();
		m_tPathPosition.Clear();
		m_tPathSpacing.Clear();
		m_tPosition.Clear();
		m_tRotation.Clear();
		m_tScale.Clear();
		m_tShear.Clear();
		m_tTransform.Clear();

		m_SlotScratch.Clear();
	}

	Bool IsDirty() const
	{
		return
			!m_vAttachments.IsEmpty() ||
			!m_tColor.IsEmpty() ||
			!m_tTwoColor.IsEmpty() ||
			!m_vDrawOrder.IsEmpty() ||
			!m_tIk.IsEmpty() ||
			!m_tPathMix.IsEmpty() ||
			!m_tPathPosition.IsEmpty() ||
			!m_tPathSpacing.IsEmpty() ||
			!m_tPosition.IsEmpty() ||
			!m_tRotation.IsEmpty() ||
			!m_tScale.IsEmpty() ||
			!m_tShear.IsEmpty() ||
			!m_tTransform.IsEmpty();
	}

	CacheSlotAttachments m_vAttachments;
	Cache4D m_tColor;
	CacheTwoColor m_tTwoColor;
	DrawOrder m_vDrawOrder;
	CacheIk m_tIk;
	Cache2D m_tPathMix;
	Cache1D m_tPathPosition;
	Cache1D m_tPathSpacing;
	Cache2D m_tPosition;
	Cache1D m_tRotation;
	Cache3D m_tScale;
	Cache2D m_tShear;
	Cache4D m_tTransform;

	// Part of processing, not public state.
	CacheSlot m_SlotScratch;
	DrawOrder m_vDrawOrderScratch;
}; // struct Cache

static inline void SetDefaultDrawOrder(UInt32 uSlots, Cache::DrawOrder& rv)
{
	rv.Resize(uSlots);
	for (UInt32 i = 0u; i < uSlots; ++i) { rv[i] = (Int16)i; }
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
