/**
 * \file Animation2DState.h
 * \brief Binds runtime posable state into the common animation
 * framework.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_STATE_H
#define ANIMATION2D_STATE_H

#include "AnimationIState.h"
#include "ScopedPtr.h"
#include "SharedPtr.h"
namespace Seoul { namespace Animation { class IData; } }
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation2D { class DataInstance; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class State SEOUL_SEALED : public Animation::IState
{
public:
	State(const Animation::IData& data, const SharedPtr<Animation::EventInterface>& pEventInterface);
	~State();

	const DataInstance& GetInstance() const { return *m_pInstance; }
	DataInstance& GetInstance() { return *m_pInstance; }
	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

private:
	SEOUL_DISABLE_COPY(State);

	ScopedPtr<DataInstance> m_pInstance;
}; // class State

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
