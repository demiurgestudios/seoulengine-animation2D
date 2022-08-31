/**
 * \file Animation2DNetworkInstance.h
 * \brief 2D animation specialization of an Animation::NetworkInstance.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_NETWORK_INSTANCE_H
#define ANIMATION2D_NETWORK_INSTANCE_H

#include "Animation2DData.h"
#include "Animation2DState.h"
#include "AnimationNetworkInstance.h"
namespace Seoul { namespace Animation2D { class Manager; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class NetworkInstance SEOUL_SEALED : public Animation::NetworkInstance
{
public:
	~NetworkInstance();

	// Instance data of this network. It is only safe to access these members when IsReady()
	// is true.
	const SharedPtr<DataDefinition const>& GetData() const
	{
		return static_cast<const Data&>(GetDataInterface()).GetPtr();
	}
	const Animation2DDataContentHandle& GetDataHandle() const
	{
		return static_cast<const Data&>(GetDataInterface()).GetHandle();
	}

	const DataInstance& GetState() const
	{
		return static_cast<const State&>(GetStateInterface()).GetInstance();
	}
	DataInstance& GetState()
	{
		return static_cast<State&>(GetStateInterface()).GetInstance();
	}

	virtual Animation::NodeInstance* CreatePlayClipInstance(
		const SharedPtr<Animation::PlayClipDefinition const>& pDef,
		const Animation::ClipSettings& settings) SEOUL_OVERRIDE;

private:
	friend class Manager;

	virtual Animation::IState* CreateState() SEOUL_OVERRIDE;
	virtual NetworkInstance* CreateClone() const SEOUL_OVERRIDE;

	NetworkInstance(
		const AnimationNetworkContentHandle& hNetwork,
		Animation::IData* pData,
		const SharedPtr<Animation::EventInterface>& pEventInterface);

	SEOUL_DISABLE_COPY(NetworkInstance);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(NetworkInstance);
}; // class NetworkInstance

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
