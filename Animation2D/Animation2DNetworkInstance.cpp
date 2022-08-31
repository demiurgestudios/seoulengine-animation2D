/**
 * \file Animation2DNetworkInstance.cpp
 * \brief 2D animation specialization of an Animation::NetworkInstance.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "AnimationIData.h"
#include "Animation2DNetworkInstance.h"
#include "Animation2DPlayClipInstance.h"
#include "Animation2DState.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

NetworkInstance::~NetworkInstance()
{
}

NetworkInstance::NetworkInstance(
	const AnimationNetworkContentHandle& hNetwork,
	Animation::IData* pData,
	const SharedPtr<Animation::EventInterface>& pEventInterface)
	: Animation::NetworkInstance(hNetwork, pData, pEventInterface)
{
}

Animation::NodeInstance* NetworkInstance::CreatePlayClipInstance(
	const SharedPtr<Animation::PlayClipDefinition const>& pDef,
	const Animation::ClipSettings& settings)
{
	return SEOUL_NEW(MemoryBudgets::Animation2D) PlayClipInstance(*this, pDef, settings);
}

Animation::IState* NetworkInstance::CreateState()
{
	return SEOUL_NEW(MemoryBudgets::Animation2D) State(GetDataInterface(), GetEventInterface());
}

NetworkInstance* NetworkInstance::CreateClone() const
{
	return SEOUL_NEW(MemoryBudgets::Animation2D) NetworkInstance(GetNetworkHandle(), GetDataInterface().Clone(), GetEventInterface());
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
