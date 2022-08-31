/**
 * \file Animation2DClipInstance.cpp
 * \brief An instance of an Animation2D::Clip. Necessary for runtime
 * playback of the clip's animation timelines.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "AnimationEventInterface.h"
#include "Animation2DCache.h"
#include "Animation2DClipDefinition.h"
#include "Animation2DClipInstance.h"
#include "Animation2DDataDefinition.h"
#include "Animation2DDataInstance.h"
#include "SeoulMath.h"

#if SEOUL_WITH_ANIMATION_2D

// TODO: All evaluators that support blending should use the Animation2D::Cache.
// Once that is complete, additive blending is straightforward.

namespace Seoul::Animation2D
{

/**
 * Time values in spine are rounded to 4 places after the decimal.
 * To make sure we hit stepped or discrete keys (e.g. attachment
 * changes) on the correct frame, we need to do the same to
 * our accumulated time values.
 */
static inline Float ToEditorTime(Float fTimeInSeconds)
{
	auto const f = Round(fTimeInSeconds * 10000.0) / 10000.0;
	return (Float)f;
}

class IEvaluator SEOUL_ABSTRACT
{
public:
	IEvaluator()
	{
	}

	virtual ~IEvaluator()
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) = 0;

private:
	SEOUL_DISABLE_COPY(IEvaluator);
}; // class IEvaluator

class DrawOrderEvaluator SEOUL_SEALED : public IEvaluator
{
public:
	DrawOrderEvaluator(DataInstance& r, const KeyFramesDrawOrder& v)
		: m_r(r)
		, m_v(v)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// TODO: Optimize as with standard KeyFrameEvalutors.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();
		while (u + 1u < uEnd && m_v[u + 1u].m_fTime <= fTime)
		{
			++u;
		}

		auto const& p = m_r.GetData();
		auto& drawOrder = m_r.GetCache().m_vDrawOrder;
		auto& scratch = m_r.GetCache().m_vDrawOrderScratch;
		auto const& v = m_v[u].m_vOffsets;

		// If no explicit draw order changes, set
		// nothing (this will commit the default).
		if (!v.IsEmpty())
		{
			// Initialize scratch.
			SetDefaultDrawOrder(m_r.GetSlots().GetSize(), scratch);

			// Clear the draw order to -1 markers initially.
			Int32 const iDraws = (Int32)scratch.GetSize();
			drawOrder.Clear();
			drawOrder.Resize((UInt32)iDraws, (Int16)-1);

			// Now walk offsets and fill in orders that are changed.
			for (auto i = v.Begin(); v.End() != i; ++i)
			{
				// For this index, insert it at its final position.
				// in the draw order, and then (temporarily) clear
				// it in the pending draw order.
				Int16 iSlot = p->GetSlotIndex(i->m_Slot);
				drawOrder[iSlot + i->m_iOffset] = iSlot;
				scratch[iSlot] = -1;
			}

			// Finally, fill in any unchanged slots, and restore
			// the pending slots, so it is always left in sequential
			// order.
			Int16 iOutSlot = (Int16)(iDraws - 1);
			for (auto i = iDraws - 1; i >= 0; --i)
			{
				// Keep decrementing iOutSlot until we hit a valid
				// slot.
				while (iOutSlot >= 0 && scratch[iOutSlot] < 0)
				{
					// Fill in pending so, when we're done, it
					// is back to being a sequential list.
					scratch[iOutSlot] = iOutSlot;
					--iOutSlot;
				}

				// If the slot was already assigned (drawOrder >= 0),
				// skip it.
				if (drawOrder[i] >= 0)
				{
					continue;
				}

				// Sanity check - if we get here, iOutSlot must be valid (>= 0);
				SEOUL_ASSERT(iOutSlot >= 0);

				// Otherwise, assign iOut.
				drawOrder[i] = iOutSlot;
				--iOutSlot;
			}

			while (iOutSlot >= 0)
			{
				// Sanity check - if we get here, pending[iOutSlot] must be invalid (< 0).
				SEOUL_ASSERT(scratch[iOutSlot] < 0);
				scratch[iOutSlot] = iOutSlot;
				--iOutSlot;
			}
		}

		// Sanity check that we properly fixed up pending, and that the sorted
		// drawOrder has all slots
#if !SEOUL_ASSERTIONS_DISABLED
		{
			auto copy = drawOrder;
			QuickSort(copy.Begin(), copy.End());
			for (UInt32 i = 0u; i < copy.GetSize(); ++i)
			{
				SEOUL_ASSERT(i == (UInt32)copy[i]);
			}
		}
		for (UInt32 i = 0u; i < scratch.GetSize(); ++i)
		{
			SEOUL_ASSERT(i == (UInt32)scratch[i]);
		}
#endif // /#if !SEOUL_ASSERTIONS_DISABLED
	}

private:
	DataInstance& m_r;
	const KeyFramesDrawOrder& m_v;

	SEOUL_DISABLE_COPY(DrawOrderEvaluator);
}; // class DrawOrderEvaluator

class EventEvaluator SEOUL_SEALED : public IEvaluator
{
public:
	EventEvaluator(
		const KeyFramesEvent& v,
		Float fEventMixThreshold)
		: m_v(v)
		, m_fEventMixThreshold(fEventMixThreshold)
	{
	}

	Bool GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const
	{
		// Find the starting index.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();

		// Iterate and search for the start time
		for (; u < uEnd; ++u)
		{
			if (m_v[u].m_fTime > fStartTime)
			{
				break;
			}
		}

		// Iterate until we find a matching event name
		for (; u < uEnd; ++u)
		{
			if (m_v[u].m_Id == sEventName)
			{
				fEventTime = m_v[u].m_fTime;
				return true;
			}
		}

		// no match
		return false;
	}

	void EvaluateRange(DataInstance& r, Float fStartTime, Float fEndTime, Float fAlpha)
	{
		// Early out if we're below the mix threshold.
		if (fAlpha < m_fEventMixThreshold)
		{
			return;
		}

		// Early out if we don't have an evaluator.
		auto pEventInterface = r.GetEventInterface();
		if (!pEventInterface.IsValid())
		{
			return;
		}

		// Find the starting index.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();

		// fStartTime == 0.0 and m_v.Front().m_fTime == 0.0f
		// is a special case. Normally, the evaluation range is (start, end], so that
		// we don't play the event at end twice (when, on the next evaluation,
		// end becomes start of the next range). However, since no time before 0.0 exists,
		// we must treat 0.0 as a special case and include it in the range.
		if (fStartTime != 0.0f || m_v.Front().m_fTime != 0.0f)
		{
			// Iterate and search for the normal start. Open range,
			// so m_v[u].m_fTime must be > fStartTime to begin evaluation
			// at it.
			for (; u < uEnd; ++u)
			{
				if (m_v[u].m_fTime > fStartTime)
				{
					break;
				}
			}
		}

		// Iterate until we hit the end, dispatch an event
		// at each frame.
		for (; u < uEnd; ++u)
		{
			// Closed range - we include an index u if its time is <= fEndTime.
			if (m_v[u].m_fTime > fEndTime)
			{
				break;
			}

			auto const& e = m_v[u];
			pEventInterface->DispatchEvent(e.m_Id, e.m_i, e.m_f, e.m_s);
		}
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// Nop
	}

private:
	const KeyFramesEvent& m_v;
	Float const m_fEventMixThreshold;

	SEOUL_DISABLE_COPY(EventEvaluator);
}; // class EventEvaluator

class KeyFrameEvaluator SEOUL_ABSTRACT : public IEvaluator
{
public:
	KeyFrameEvaluator(const BezierCurves& vCurves)
		: m_vCurves(vCurves)
		, m_uLastKeyFrame(0)
	{
	}

	virtual ~KeyFrameEvaluator()
	{
	}

	template <typename T>
	Float32 GetAlpha(
		Float fTime,
		const T& r0,
		const T& r1) const
	{
		switch (r0.GetCurveType())
		{
		case CurveType::kLinear:
			return Clamp((fTime - r0.m_fTime) / (r1.m_fTime - r0.m_fTime), 0.0f, 1.0f);
		case CurveType::kStepped:
			return 0.0f;
		case CurveType::kBezier:
			return GetBezierCurveAlpha(
				Clamp((fTime - r0.m_fTime) / (r1.m_fTime - r0.m_fTime), 0.0f, 1.0f),
				m_vCurves[r0.m_uCurveDataOffset]);
		default:
			SEOUL_FAIL("Out-of-sync enum, programmer error.");
			return 0.0f;
		};
	}

	template <typename T>
	Float32 GetFrames(
		const T& v,
		Float fTime,
		typename T::ValueType const*& pr0,
		typename T::ValueType const*& pr1)
	{
		SEOUL_ASSERT(!v.IsEmpty());

		if (v[m_uLastKeyFrame].m_fTime > fTime)
		{
			if (0u == m_uLastKeyFrame)
			{
				pr0 = v.Get(m_uLastKeyFrame);
				pr1 = pr0;

				return GetAlpha(fTime, *pr0, *pr1);
			}

			m_uLastKeyFrame = 0u;
		}

		UInt32 const uSize = v.GetSize();
		for (; m_uLastKeyFrame + 1u < uSize; ++m_uLastKeyFrame)
		{
			if (v[m_uLastKeyFrame + 1u].m_fTime > fTime)
			{
				pr0 = v.Get(m_uLastKeyFrame);
				pr1 = v.Get(m_uLastKeyFrame + 1u);

				return GetAlpha(fTime, *pr0, *pr1);
			}
		}

		pr0 = v.Get(m_uLastKeyFrame);
		pr1 = pr0;

		return GetAlpha(fTime, *pr0, *pr1);
	}

private:
	const BezierCurves& m_vCurves;
	UInt32 m_uLastKeyFrame;

	static Float32 GetBezierCurveAlpha(Float fLinearAlpha, const BezierCurve& a)
	{
		Float fX = a[0];
		if (fX >= fLinearAlpha)
		{
			return (a[1] * fLinearAlpha) / fX;
		}

		for (UInt32 i = 2u; i < a.GetSize(); i += 2u)
		{
			fX = a[i];
			if (fX >= fLinearAlpha)
			{
				Float const fPrevX = a[i-2];
				Float const fPrevY = a[i-1];
				return fPrevY + (((a[i+1] - fPrevY) * (fLinearAlpha - fPrevX)) / (fX - fPrevX));
			}
		}

		Float const fY = a.Back();
		return fY + (((1.0f - fY) * (fLinearAlpha - fX)) / (1.0f - fX));
	}

	SEOUL_DISABLE_COPY(KeyFrameEvaluator);
}; // class KeyFrameEvaluator

class DeformEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	DeformEvaluator(
		DataInstance& r,
		const KeyFramesDeform& v,
		const BezierCurves& vCurves,
		const DeformKey& key)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_Key(key)
	{
		auto& rRefs = m_r.GetDeformReferences();
		auto p = rRefs.Find(m_Key);
		if (nullptr == p)
		{
			SEOUL_VERIFY(rRefs.Insert(m_Key, 1).Second);
		}
		else
		{
			(*p)++;
		}
	}

	~DeformEvaluator()
	{
		auto& rRefs = m_r.GetDeformReferences();
		auto p = rRefs.Find(m_Key);
		SEOUL_ASSERT(nullptr != p);
		SEOUL_ASSERT(*p > 0);
		--(*p);

		if (*p == 0)
		{
			SEOUL_VERIFY(rRefs.Erase(m_Key));
			CheckedPtr<DataInstance::DeformData> p;
			(void)m_r.GetDeforms().GetValue(m_Key, p);
			(void)m_r.GetDeforms().Erase(m_Key);
			SafeDelete(p);
		}
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime)
		{
			auto& rRefs = m_r.GetDeformReferences();
			auto p = rRefs.Find(m_Key);
			SEOUL_ASSERT(nullptr != p);
			if (*p == 1)
			{
				CheckedPtr<DataInstance::DeformData> pDeform;
				(void)m_r.GetDeforms().GetValue(m_Key, pDeform);
				(void)m_r.GetDeforms().Erase(m_Key);
				SafeDelete(pDeform);
			}

			return;
		}

		KeyFrameDeform const* pk0 = nullptr;
		KeyFrameDeform const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		auto const& v0 = (pk0->m_vVertices);
		auto const& v1 = (pk1->m_vVertices);

		// Sanity check, this should be enforced by the data loader.
		SEOUL_ASSERT(v0.GetSize() == v1.GetSize());

		auto& deforms = m_r.GetDeforms();
		CheckedPtr<DataInstance::DeformData> pData;
		if (!deforms.GetValue(m_Key, pData))
		{
			pData = SEOUL_NEW(MemoryBudgets::Animation2D) DataInstance::DeformData(v0.GetSize());
			SEOUL_VERIFY(deforms.Insert(m_Key, pData).Second);

			// Since we are initializing the data for the first time, don't
			// want to blend.
			fAlpha = 1.0f;
		}

		// Perform the actual interpolation. Two different loops to avoid
		// some extra work when fAlpha < 1.0f.
		auto& vOut = *pData;
		UInt32 const uSize = vOut.GetSize();
		if (fAlpha < 1.0f)
		{
			for (UInt32 i = 0u; i < uSize; ++i)
			{
				vOut[i] += (Lerp(v0[i], v1[i], fT) - vOut[i]) * fAlpha;
			}
		}
		else
		{
			for (UInt32 i = 0u; i < uSize; ++i)
			{
				vOut[i] = Lerp(v0[i], v1[i], fT);
			}
		}
	}

private:
	DataInstance& m_r;
	const KeyFramesDeform& m_v;
	DeformKey const m_Key;

	SEOUL_DISABLE_COPY(DeformEvaluator);
}; // class DeformEvaluator

static Float32 LerpBoolean(Bool bBase, Bool b0, Bool b1, Float fT, Float fAlpha)
{
	auto const fB = (bBase ? 1.0f : 0.0f);
	auto const f0 = (b0 ? 1.0f : 0.0f);
	auto const f1 = (b1 ? 1.0f : 0.0f);

	return  (Lerp(f0, f1, fT) - fB) * fAlpha;
}

class IkEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	IkEvaluator(
		DataInstance& r,
		const KeyFramesIk& v,
		const BezierCurves& vCurves,
		Int16 iIk)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iIk(iIk)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetIk()[m_iIk];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameIk const* pk0 = nullptr;
		KeyFrameIk const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Setup values - these are deltas from the t-pose value.
		Cache::IkEntry entry;
		entry.m_fMix = (Lerp(pk0->m_fMix, pk1->m_fMix, fT) - base.m_fMix) * fAlpha;
		entry.m_fSoftness = (Lerp(pk0->m_fSoftness, pk1->m_fSoftness, fT) - base.m_fSoftness) * fAlpha;
		entry.m_fBendPositive = LerpBoolean(base.m_bBendPositive, pk0->m_bBendPositive, pk1->m_bBendPositive, fT, fAlpha);
		entry.m_fCompress = LerpBoolean(base.m_bCompress, pk0->m_bCompress, pk1->m_bCompress, fT, fAlpha);
		entry.m_fStretch = LerpBoolean(base.m_bStretch, pk0->m_bStretch, pk1->m_bStretch, fT, fAlpha);

		// Accumulate.
		cache.AccumIk(m_iIk, entry);
	}

private:
	DataInstance& m_r;
	const KeyFramesIk& m_v;
	Int16 const m_iIk;

	SEOUL_DISABLE_COPY(IkEvaluator);
}; // class IkEvaluator

class PathMixEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	PathMixEvaluator(
		DataInstance& r,
		const KeyFramesPathMix& v,
		const BezierCurves& vCurves,
		Int16 iPath)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iPath(iPath)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetPaths()[m_iPath];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFramePathMix const* pk0 = nullptr;
		KeyFramePathMix const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumPathMix(m_iPath, Vector2D(
			(Lerp(pk0->m_fPositionMix, pk1->m_fPositionMix, fT) - base.m_fPositionMix) * fAlpha,
			(Lerp(pk0->m_fRotationMix, pk1->m_fRotationMix, fT) - base.m_fRotationMix) * fAlpha));
	}

private:
	DataInstance& m_r;
	const KeyFramesPathMix& m_v;
	Int16 const m_iPath;

	SEOUL_DISABLE_COPY(PathMixEvaluator);
}; // class PathMixEvaluator

class PathPositionEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	PathPositionEvaluator(
		DataInstance& r,
		const KeyFramesPathPosition& v,
		const BezierCurves& vCurves,
		Int16 iPath)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iPath(iPath)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetPaths()[m_iPath];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFramePathPosition const* pk0 = nullptr;
		KeyFramePathPosition const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumPathPosition(m_iPath,
			(Lerp(pk0->m_fPosition, pk1->m_fPosition, fT) - base.m_fPosition) * fAlpha);
	}

private:
	DataInstance& m_r;
	const KeyFramesPathPosition& m_v;
	Int16 const m_iPath;

	SEOUL_DISABLE_COPY(PathPositionEvaluator);
}; // class PathPositionEvaluator

class PathSpacingEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	PathSpacingEvaluator(
		DataInstance& r,
		const KeyFramesPathSpacing& v,
		const BezierCurves& vCurves,
		Int16 iPath)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iPath(iPath)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetPaths()[m_iPath];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFramePathSpacing const* pk0 = nullptr;
		KeyFramePathSpacing const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumPathSpacing(m_iPath,
			(Lerp(pk0->m_fSpacing, pk1->m_fSpacing, fT) - base.m_fSpacing) * fAlpha);
	}

private:
	DataInstance& m_r;
	const KeyFramesPathSpacing& m_v;
	Int16 const m_iPath;

	SEOUL_DISABLE_COPY(PathSpacingEvaluator);
}; // class PathSpacingEvaluator

class RotationEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	RotationEvaluator(
		DataInstance& r,
		const KeyFramesRotation& v,
		const BezierCurves& vCurves,
		Int16 iBone)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameRotation const* pk0 = nullptr;
		KeyFrameRotation const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumRotation(m_iBone,
			fAlpha * LerpDegrees(pk0->m_fAngleInDegrees, pk1->m_fAngleInDegrees, fT));
	}

private:
	DataInstance& m_r;
	const KeyFramesRotation& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(RotationEvaluator);
}; // class RotationEvaluator

class ScaleEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	ScaleEvaluator(
		DataInstance& r,
		const KeyFramesScale& v,
		const BezierCurves& vCurves,
		Int16 iBone)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameScale const* pk0 = nullptr;
		KeyFrameScale const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumScale(m_iBone,
			fAlpha * Vector2D(Lerp(pk0->m_fX, pk1->m_fX, fT), Lerp(pk0->m_fY, pk1->m_fY, fT)), fAlpha);
	}

private:
	DataInstance& m_r;
	const KeyFramesScale& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(ScaleEvaluator);
}; // class ScaleEvaluator

class ShearEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	ShearEvaluator(
		DataInstance& r,
		const KeyFrames2D& v,
		const BezierCurves& vCurves,
		Int16 iBone)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrame2D const* pk0 = nullptr;
		KeyFrame2D const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumShear(m_iBone,
			fAlpha * Vector2D(Lerp(pk0->m_fX, pk1->m_fX, fT), Lerp(pk0->m_fY, pk1->m_fY, fT)));
	}

private:
	DataInstance& m_r;
	const KeyFrames2D& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(ShearEvaluator);
}; // class ShearEvaluator

class SlotAttachmentEvaluator SEOUL_SEALED : public IEvaluator
{
public:
	SlotAttachmentEvaluator(
		DataInstance& r,
		const KeyFramesAttachment& v,
		Int16 iSlot)
		: m_r(r)
		, m_v(v)
		, m_iSlot(iSlot)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }
		// Potentially don't apply based on blend mode (mis)match.
		if (!bBlendDiscreteState && 1.0f != fAlpha) { return; }

		// TODO: Optimize as with standard KeyFrameEvalutors.
		UInt32 u = 0u;
		UInt32 const uEnd = m_v.GetSize();
		while (u + 1u < uEnd && m_v[u + 1u].m_fTime <= fTime)
		{
			++u;
		}

		// Accumulate.
		auto& cache = m_r.GetCache();
		cache.AccumSlotAttachment(m_iSlot, m_v[u].m_Id, fAlpha);
	}

private:
	DataInstance& m_r;
	const KeyFramesAttachment& m_v;
	Int16 const m_iSlot;

	SEOUL_DISABLE_COPY(SlotAttachmentEvaluator);
}; // class SlotAttachmentEvaluator

class SlotColorEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	SlotColorEvaluator(
		DataInstance& r,
		const KeyFramesColor& v,
		const BezierCurves& vCurves,
		Int16 iSlot)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iSlot(iSlot)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetSlots()[m_iSlot];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameColor const* pk0 = nullptr;
		KeyFrameColor const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumSlotColor(m_iSlot, Vector4D(
			(Lerp((Float)pk0->m_Color.m_R, (Float)pk1->m_Color.m_R, fT) - (Float)base.m_Color.m_R) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_G, (Float)pk1->m_Color.m_G, fT) - (Float)base.m_Color.m_G) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_B, (Float)pk1->m_Color.m_B, fT) - (Float)base.m_Color.m_B) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_A, (Float)pk1->m_Color.m_A, fT) - (Float)base.m_Color.m_A) * fAlpha));
	}

private:
	DataInstance& m_r;
	const KeyFramesColor& m_v;
	Int16 const m_iSlot;

	SEOUL_DISABLE_COPY(SlotColorEvaluator);
}; // class SlotColorEvaluator

class SlotTwoColorEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	SlotTwoColorEvaluator(
		DataInstance& r,
		const KeyFramesTwoColor& v,
		const BezierCurves& vCurves,
		Int16 iSlot)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iSlot(iSlot)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetSlots()[m_iSlot];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameTwoColor const* pk0 = nullptr;
		KeyFrameTwoColor const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumSlotTwoColor(m_iSlot, Cache::TwoColorEntry(
			(Lerp((Float)pk0->m_Color.m_R, (Float)pk1->m_Color.m_R, fT) - (Float)base.m_Color.m_R) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_G, (Float)pk1->m_Color.m_G, fT) - (Float)base.m_Color.m_G) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_B, (Float)pk1->m_Color.m_B, fT) - (Float)base.m_Color.m_B) * fAlpha,
			(Lerp((Float)pk0->m_Color.m_A, (Float)pk1->m_Color.m_A, fT) - (Float)base.m_Color.m_A) * fAlpha,
			(Lerp((Float)pk0->m_SecondaryColor.m_R, (Float)pk1->m_SecondaryColor.m_R, fT) - (Float)base.m_SecondaryColor.m_R) * fAlpha,
			(Lerp((Float)pk0->m_SecondaryColor.m_G, (Float)pk1->m_SecondaryColor.m_G, fT) - (Float)base.m_SecondaryColor.m_G) * fAlpha,
			(Lerp((Float)pk0->m_SecondaryColor.m_B, (Float)pk1->m_SecondaryColor.m_B, fT) - (Float)base.m_SecondaryColor.m_B) * fAlpha));
	}

private:
	DataInstance& m_r;
	const KeyFramesTwoColor& m_v;
	Int16 const m_iSlot;

	SEOUL_DISABLE_COPY(SlotTwoColorEvaluator);
}; // class SlotTwoColorEvaluator

class TransformEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	TransformEvaluator(
		DataInstance& r,
		const KeyFramesTransform& v,
		const BezierCurves& vCurves,
		Int16 iTransform)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iTransform(iTransform)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto const& p = m_r.GetData();
		auto const& base = p->GetTransforms()[m_iTransform];
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrameTransform const* pk0 = nullptr;
		KeyFrameTransform const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumTransform(m_iTransform, Vector4D(
			(Lerp(pk0->m_fPositionMix, pk1->m_fPositionMix, fT) - base.m_fPositionMix) * fAlpha,
			(Lerp(pk0->m_fRotationMix, pk1->m_fRotationMix, fT) - base.m_fRotationMix) * fAlpha,
			(Lerp(pk0->m_fScaleMix, pk1->m_fScaleMix, fT) - base.m_fScaleMix) * fAlpha,
			(Lerp(pk0->m_fShearMix, pk1->m_fShearMix, fT) - base.m_fShearMix) * fAlpha));
	}

private:
	DataInstance& m_r;
	const KeyFramesTransform& m_v;
	Int16 const m_iTransform;

	SEOUL_DISABLE_COPY(TransformEvaluator);
}; // class TransformEvaluator

class TranslationEvaluator SEOUL_SEALED : public KeyFrameEvaluator
{
public:
	TranslationEvaluator(
		DataInstance& r,
		const KeyFrames2D& v,
		const BezierCurves& vCurves,
		Int16 iBone)
		: KeyFrameEvaluator(vCurves)
		, m_r(r)
		, m_v(v)
		, m_iBone(iBone)
	{
	}

	virtual void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE
	{
		// If prior to the start of the curve, don't apply.
		if (fTime < m_v.Front().m_fTime) { return; }

		// Cache common values.
		auto& cache = m_r.GetCache();

		// Standard case, interpolate between frames.
		KeyFrame2D const* pk0 = nullptr;
		KeyFrame2D const* pk1 = nullptr;
		Float const fT = GetFrames(m_v, fTime, pk0, pk1);

		// Accumulate.
		cache.AccumPosition(m_iBone,
			fAlpha * Vector2D(Lerp(pk0->m_fX, pk1->m_fX, fT), Lerp(pk0->m_fY, pk1->m_fY, fT)));
	}

private:
	DataInstance& m_r;
	const KeyFrames2D& m_v;
	Int16 const m_iBone;

	SEOUL_DISABLE_COPY(TranslationEvaluator);
}; // class TranslationEvaluator

ClipInstance::ClipInstance(DataInstance& r, const SharedPtr<Clip>& pClip, const Animation::ClipSettings& settings)
	: m_Settings(settings)
	, m_r(r)
	, m_pClip(pClip)
	, m_fMaxTime(0.0f)
	, m_vEvaluators()
	, m_pEventEvaluator()
{
	InternalConstructEvaluators();
}

ClipInstance::~ClipInstance()
{
	m_pEventEvaluator.Reset();
	SafeDeleteVector(m_vEvaluators);
}

Bool ClipInstance::GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const
{
	// Sanitize.
	fStartTime = ToEditorTime(fStartTime);

	if (m_pEventEvaluator.IsValid())
	{
		return m_pEventEvaluator->GetNextEventTime(sEventName, fStartTime, fEventTime);
	}

	return false;
}

/**
 * Used for event dispatch, pass a time range. Looping should be implemented
 * by passing all time ranges (where fPrevTime >= 0.0f and fTime <= GetMaxTime())
 * iteratively until the final time is reached.
 */
void ClipInstance::EvaluateRange(Float fStartTime, Float fEndTime, Float fAlpha)
{
	// Sanitize.
	fStartTime = ToEditorTime(fStartTime);
	fEndTime = ToEditorTime(fEndTime);

	if (m_pEventEvaluator.IsValid())
	{
		m_pEventEvaluator->EvaluateRange(m_r, fStartTime, fEndTime, fAlpha);
	}
}

void ClipInstance::Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState)
{
	// Sanitize.
	fTime = ToEditorTime(fTime);

	for (auto i = m_vEvaluators.Begin(); m_vEvaluators.End() != i; ++i)
	{
		(*i)->Evaluate(fTime, fAlpha, bBlendDiscreteState);
	}
}

void ClipInstance::InternalConstructEvaluators()
{
	SafeDeleteVector(m_vEvaluators);

	auto const& pData = m_r.GetData();

	// Bones first.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t = m_pClip->GetBones();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + (t.GetSize() * 4u));

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& entry = i->Second;
			auto const iBone = pData->GetBoneIndex(i->First);

			// Skip entries if no bone is available. This supports retargeting.
			if (iBone < 0)
			{
				continue;
			}

			if (!entry.m_vRotation.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vRotation.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) RotationEvaluator(m_r, entry.m_vRotation, vCurves, iBone));
			}
			if (!entry.m_vScale.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vScale.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) ScaleEvaluator(m_r, entry.m_vScale, vCurves, iBone));
			}
			if (!entry.m_vShear.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vShear.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) ShearEvaluator(m_r, entry.m_vShear, vCurves, iBone));
			}
			if (!entry.m_vTranslation.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vTranslation.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) TranslationEvaluator(m_r, entry.m_vTranslation, vCurves, iBone));
			}
		}
	}

	// Now deforms.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t0 = m_pClip->GetDeforms();

		auto const iBegin0 = t0.Begin();
		auto const iEnd0 = t0.End();
		for (auto i0 = iBegin0; iEnd0 != i0; ++i0)
		{
			auto const& t1 = i0->Second;
			auto const iBegin1 = t1.Begin();
			auto const iEnd1 = t1.End();
			for (auto i1 = iBegin1; iEnd1 != i1; ++i1)
			{
				auto const& t2 = i1->Second;
				auto const iBegin2 = t2.Begin();
				auto const iEnd2 = t2.End();
				for (auto i2 = iBegin2; iEnd2 != i2; ++i2)
				{
					auto const& v = i2->Second;
					m_fMaxTime = Max(m_fMaxTime, v.Back().m_fTime);

					auto const key = DeformKey(i0->First, i1->First, i2->First);
					m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) DeformEvaluator(m_r, v, vCurves, key));
				}
			}
		}
	}

	// Draw order next.
	{
		auto const& v = m_pClip->GetDrawOrder();
		if (!v.IsEmpty())
		{
			m_fMaxTime = Max(m_fMaxTime, v.Back().m_fTime);
			m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) DrawOrderEvaluator(m_r, v));
		}
	}

	// Events next.
	{
		auto const& v = m_pClip->GetEvents();
		if (!v.IsEmpty())
		{
			m_fMaxTime = Max(m_fMaxTime, v.Back().m_fTime);

			auto pEventEvaluator(SEOUL_NEW(MemoryBudgets::Animation2D) EventEvaluator(v, m_Settings.m_fEventMixThreshold));
			m_vEvaluators.PushBack(pEventEvaluator);
			m_pEventEvaluator = pEventEvaluator;
		}
	}

	// Now ik.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t = m_pClip->GetIk();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + t.GetSize());

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& entry = i->Second;
			auto const iIk = pData->GetIkIndex(i->First);

			m_fMaxTime = Max(m_fMaxTime, entry.Back().m_fTime);
			m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) IkEvaluator(m_r, entry, vCurves, iIk));
		}
	}

	// Now paths.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t = m_pClip->GetPaths();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + (t.GetSize() * 4u));

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& entry = i->Second;
			auto const iPath = pData->GetPathIndex(i->First);

			if (!entry.m_vMix.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vMix.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) PathMixEvaluator(m_r, entry.m_vMix, vCurves, iPath));
			}
			if (!entry.m_vPosition.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vPosition.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) PathPositionEvaluator(m_r, entry.m_vPosition, vCurves, iPath));
			}
			if (!entry.m_vSpacing.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vSpacing.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) PathSpacingEvaluator(m_r, entry.m_vSpacing, vCurves, iPath));
			}
		}
	}

	// Now slots.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t = m_pClip->GetSlots();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + (t.GetSize() * 4u));

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& entry = i->Second;
			auto const iSlot = pData->GetSlotIndex(i->First);

			if (!entry.m_vAttachment.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vAttachment.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) SlotAttachmentEvaluator(m_r, entry.m_vAttachment, iSlot));
			}
			if (!entry.m_vColor.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vColor.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) SlotColorEvaluator(m_r, entry.m_vColor, vCurves, iSlot));
			}
			if (!entry.m_vTwoColor.IsEmpty())
			{
				m_fMaxTime = Max(m_fMaxTime, entry.m_vTwoColor.Back().m_fTime);
				m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) SlotTwoColorEvaluator(m_r, entry.m_vTwoColor, vCurves, iSlot));
			}
		}
	}

	// Finally, transforms.
	{
		auto const& vCurves = m_r.GetData()->GetCurves();
		auto const& t = m_pClip->GetTransforms();
		m_vEvaluators.Reserve(m_vEvaluators.GetSize() + t.GetSize());

		auto const iBegin = t.Begin();
		auto const iEnd = t.End();
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto const& v = i->Second;
			auto const iTransform = pData->GetTransformIndex(i->First);

			m_fMaxTime = Max(m_fMaxTime, v.Back().m_fTime);
			m_vEvaluators.PushBack(SEOUL_NEW(MemoryBudgets::Animation2D) TransformEvaluator(m_r, v, vCurves, iTransform));
		}
	}
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
