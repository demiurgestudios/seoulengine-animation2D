/**
 * \file Animation2DClipInstance.h
 * \brief An instance of an Animation2D::Clip. Necessary for runtime
 * playback of the clip's animation timelines.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_CLIP_INSTANCE_H
#define ANIMATION2D_CLIP_INSTANCE_H

#include "AnimationClipSettings.h"
#include "CheckedPtr.h"
#include "Prereqs.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Animation2D { class Clip; } }
namespace Seoul { namespace Animation2D { class IEvaluator; } }
namespace Seoul { namespace Animation2D { class DataInstance; } }
namespace Seoul { namespace Animation2D { class EventEvaluator; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class ClipInstance SEOUL_SEALED
{
public:
	ClipInstance(DataInstance& r, const SharedPtr<Clip>& pClip, const Animation::ClipSettings& settings);
	~ClipInstance();

	// The number of active animation evaluators in this clip.
	UInt32 GetActiveEvaluatorCount() const { return m_vEvaluators.GetSize(); }

	// Used for event dispatch, pass a time range. Looping should be implemented
	// by passing all time ranges (where fPrevTime >= 0.0f and fTime <= GetMaxTime())
	// iteratively until the final time is reached.
	void EvaluateRange(Float fStartTime, Float fEndTime, Float fAlpha);

	// Apply the clip to the state of DataInstance.
	void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState);

	/** @return The max time (in seconds) of all timelines in this animation clip. */
	Float GetMaxTime() const { return m_fMaxTime; }

	// returns true if the animation event was found after the current animation time, and sets the event time
	// returns false if the animation event was not found
	Bool GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const;

private:
	const Animation::ClipSettings m_Settings;
	DataInstance& m_r;
	SharedPtr<Clip> m_pClip;
	Float m_fMaxTime;

	typedef Vector<CheckedPtr<IEvaluator>, MemoryBudgets::Animation2D> Evaluators;
	Evaluators m_vEvaluators;
	CheckedPtr<EventEvaluator> m_pEventEvaluator;

	void InternalConstructEvaluators();

	SEOUL_DISABLE_COPY(ClipInstance);
}; // class ClipInstance

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
