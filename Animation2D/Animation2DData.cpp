/**
* \file Animation2DData.cpp
* \brief Binds runtime data ptr into the common animation
* framework.
*
* Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
*/

#include "Animation2DData.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

Data::Data(const Animation2DDataContentHandle& hData)
	: m_hData(hData)
	, m_pData()
{
}

Data::~Data()
{
}

void Data::AcquireInstance()
{
	m_pData = m_hData.GetPtr();
}

Animation::IData* Data::Clone() const
{
	return SEOUL_NEW(MemoryBudgets::Animation2D) Data(m_hData);
}

Atomic32Type Data::GetTotalLoadsCount() const
{
	return m_hData.GetTotalLoadsCount();
}

Bool Data::HasInstance() const
{
	return m_pData.IsValid();
}

Bool Data::IsLoading() const
{
	return m_hData.IsLoading();
}

void Data::ReleaseInstance()
{
	m_pData.Reset();
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
