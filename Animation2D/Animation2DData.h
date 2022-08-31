/**
 * \file Animation2DData.h
 * \brief Binds runtime data ptr into the common animation
 * framework.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_DATA_H
#define ANIMATION2D_DATA_H

#include "Animation2DDataDefinition.h"
#include "AnimationIData.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class Data SEOUL_SEALED : public Animation::IData
{
public:
	Data(const Animation2DDataContentHandle& hData);
	~Data();

	virtual void AcquireInstance() SEOUL_OVERRIDE;
	virtual IData* Clone() const SEOUL_OVERRIDE;
	virtual Atomic32Type GetTotalLoadsCount() const SEOUL_OVERRIDE;
	virtual Bool HasInstance() const SEOUL_OVERRIDE;
	virtual Bool IsLoading() const SEOUL_OVERRIDE;
	virtual void ReleaseInstance() SEOUL_OVERRIDE;

	const Animation2DDataContentHandle& GetHandle() const
	{
		return m_hData;
	}
	const SharedPtr<DataDefinition const>& GetPtr() const
	{
		return m_pData;
	}

private:
	SEOUL_DISABLE_COPY(Data);

	Animation2DDataContentHandle const m_hData;
	SharedPtr<DataDefinition const> m_pData;
}; // class Data

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
