/**
 * \file Animation2DManager.h
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_MANAGER_H
#define ANIMATION2D_MANAGER_H

#include "AnimationNetworkDefinition.h"
#include "Animation2DDataDefinition.h"
#include "ContentStore.h"
#include "Delegate.h"
#include "Singleton.h"
namespace Seoul { namespace Animation { class EventInterface; } }
namespace Seoul { namespace Animation2D { class NetworkInstance; } }

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class Manager SEOUL_SEALED : public Singleton<Manager>
{
public:
	typedef Delegate<void(HString name)> EventCallback;
	typedef Vector<SharedPtr<Animation2D::NetworkInstance>, MemoryBudgets::Animation2D> Instances;

	Manager();
	~Manager();

	/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
	SharedPtr<Animation2D::NetworkInstance> CreateInstance(
		const AnimationNetworkContentHandle& hNetwork,
		const Animation2DDataContentHandle& hData,
		const SharedPtr<Animation::EventInterface>& pEventInterface);
	SharedPtr<Animation2D::NetworkInstance> CreateInstance(
		FilePath networkFilePath,
		FilePath dataFilePath,
		const SharedPtr<Animation::EventInterface>& pEventInterface);

	/**
	 * @return A persistent Content::Handle<> to the data filePath.
	 */
	Animation2DDataContentHandle GetData(FilePath filePath)
	{
		return m_DataContent.GetContent(filePath);
	}

	// Get a copy of the current list of network instances.
	void GetActiveNetworkInstances(Instances& rvInstances) const;

	// Per-frame maintenance.
	void Tick(Float fDeltaTimeInSeconds);

private:
	friend class DataContentLoader;
	Content::Store<DataDefinition> m_DataContent;

#if !SEOUL_SHIP
	Instances m_vInstances;
	Mutex m_Mutex;
#endif // /#if !SEOUL_SHIP

	SEOUL_DISABLE_COPY(Manager);
}; // class Manager

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
