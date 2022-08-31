/**
 * \file Animation2DContentLoader.h
 * \brief Specialization of Content::LoaderBase for loading animation
 * data and animation network data.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#pragma once
#ifndef ANIMATION2D_CONTENT_LOADER_H
#define ANIMATION2D_CONTENT_LOADER_H

#include "Animation2DDataDefinition.h"
#include "ContentLoaderBase.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

class DataContentLoader SEOUL_SEALED : public Content::LoaderBase
{
public:
	DataContentLoader(FilePath filePath, const Animation2DDataContentHandle& hEntry);
	~DataContentLoader();

protected:
	virtual Content::LoadState InternalExecuteContentLoadOp() SEOUL_OVERRIDE;

private:
	void InternalFreeData();
	void InternalReleaseEntry();

	Animation2DDataContentHandle m_hEntry;
	void* m_pRawData;
	UInt32 m_uDataSizeInBytes;
	Bool m_bNetworkPrefetched;
}; // class DataContentLoader

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D

#endif // include guard
