/**
 * \file Animation2DState.cpp
 * \brief Binds runtime posable state into the common animation
 * framework.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "AnimationNetworkInstance.h"
#include "Animation2DData.h"
#include "Animation2DDataInstance.h"
#include "Animation2DState.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

State::State(const Animation::IData& data, const SharedPtr<Animation::EventInterface>& pEventInterface)
	: m_pInstance(SEOUL_NEW(MemoryBudgets::Animation2D) DataInstance(static_cast<const Data&>(data).GetPtr(), pEventInterface))
{
}

State::~State()
{
}

void State::Tick(Float fDeltaTimeInSeconds)
{
	// Apply the animation cache prior to pose.
	m_pInstance->ApplyCache();

	// TODO: Break this out into a separate Pose(), so it
	// is only called if actually rendering a frame.
	m_pInstance->PoseSkinningPalette();
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
