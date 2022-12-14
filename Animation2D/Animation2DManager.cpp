/**
 * \file Animation2DManager.cpp
 * \brief Global singleton that manages animation and network data in
 * the SeoulEngine content system.
 *
 * Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "AnimationNetworkDefinitionManager.h"
#include "Animation2DData.h"
#include "Animation2DManager.h"
#include "Animation2DNetworkInstance.h"
#include "Animation2DState.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

Manager::Manager()
	: m_DataContent()
#if !SEOUL_SHIP
	, m_vInstances()
	, m_Mutex()
#endif // /#if !SEOUL_SHIP
{
}

Manager::~Manager()
{
}

/** Get a copy of the current list of network instances. */
void Manager::GetActiveNetworkInstances(Instances& rvInstances) const
{
#if !SEOUL_SHIP
	Lock lock(m_Mutex);
	rvInstances = m_vInstances;
#endif // /#if !SEOUL_SHIP
}

/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
SharedPtr<Animation2D::NetworkInstance> Manager::CreateInstance(
	const AnimationNetworkContentHandle& hNetwork,
	const Animation2DDataContentHandle& hData,
	const SharedPtr<Animation::EventInterface>& pEventInterface)
{
	auto pData(SEOUL_NEW(MemoryBudgets::Animation2D) Data(hData));
	SharedPtr<Animation2D::NetworkInstance> pReturn(SEOUL_NEW(MemoryBudgets::Animation2D) NetworkInstance(
		hNetwork,
		pData,
		pEventInterface));

	// Track the instance in developer builds.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		m_vInstances.PushBack(pReturn);
	}
#endif // /#if !SEOUL_SHIP

	return pReturn;
}

/** @return A new network instance. In development builds, instances are tracked for debugging purposes. */
SharedPtr<Animation2D::NetworkInstance> Manager::CreateInstance(
	FilePath networkFilePath,
	FilePath dataFilePath,
	const SharedPtr<Animation::EventInterface>& pEventInterface)
{
	auto pData(SEOUL_NEW(MemoryBudgets::Animation2D) Data(GetData(dataFilePath)));
	SharedPtr<Animation2D::NetworkInstance> pReturn(SEOUL_NEW(MemoryBudgets::Animation2D) NetworkInstance(
		Animation::NetworkDefinitionManager::Get()->GetNetwork(networkFilePath),
		pData,
		pEventInterface));

	// Track the instance in developer builds.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		m_vInstances.PushBack(pReturn);
	}
#endif // /#if !SEOUL_SHIP

	return pReturn;
}

/** Per-frame maintenance. */
void Manager::Tick(Float fDeltaTimeInSeconds)
{
	// Prune stale instances.
#if !SEOUL_SHIP
	{
		Lock lock(m_Mutex);
		Int32 iCount = (Int32)m_vInstances.GetSize();
		for (Int32 i = 0; i < iCount; ++i)
		{
			if (m_vInstances[i].IsUnique())
			{
				Swap(m_vInstances[i], m_vInstances[iCount-1]);
				--iCount;
				--i;
			}
		}
		m_vInstances.Resize((UInt32)iCount);
	}
#endif // /#if !SEOUL_SHIP
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
