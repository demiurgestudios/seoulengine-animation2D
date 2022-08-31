/**
 * \file Animation2DContentLoader.cpp
 * \brief Specialization of Content::LoaderBase for loading animation
 * data and animation network data.
 *
 * Copyright (c) 2017-2022 Demiurge Studios Inc. All rights reserved.
 */

#include "Animation2DContentLoader.h"
#include "Animation2DReadWriteUtil.h"
#include "Compress.h"
#include "CookManager.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "FileManager.h"

#if SEOUL_WITH_ANIMATION_2D

namespace Seoul::Animation2D
{

DataContentLoader::DataContentLoader(FilePath filePath, const Animation2DDataContentHandle& hEntry)
	: Content::LoaderBase(filePath)
	, m_hEntry(hEntry)
	, m_pRawData(nullptr)
	, m_uDataSizeInBytes(0u)
	, m_bNetworkPrefetched(false)
{
	m_hEntry.GetContentEntry()->IncrementLoaderCount();

	// Kick off prefetching of the asset (this will be a nop for local files)
	m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(filePath);
}

DataContentLoader::~DataContentLoader()
{
	// Block until this Content::LoaderBase is in a non-loading state.
	WaitUntilContentIsNotLoading();

	InternalReleaseEntry();
	InternalFreeData();
}

Content::LoadState DataContentLoader::InternalExecuteContentLoadOp()
{
	// First step, load the data.
	if (Content::LoadState::kLoadingOnFileIOThread == GetContentLoadState())
	{
		// If we're the only reference to the content, "cancel" the load.
		if (m_hEntry.IsUnique())
		{
			m_hEntry.GetContentEntry()->CancelLoad();
			InternalReleaseEntry();
			return Content::LoadState::kLoaded;
		}

		// Only try to read from disk. Let the prefetch finish the download.
		if (FileManager::Get()->IsServicedByNetwork(GetFilePath()))
		{
			if (FileManager::Get()->IsNetworkFileIOEnabled())
			{
				// Kick off a prefetch if we have not yet done so.
				if (!m_bNetworkPrefetched)
				{
					m_bNetworkPrefetched = FileManager::Get()->NetworkPrefetch(GetFilePath());
				}

				return Content::LoadState::kLoadingOnFileIOThread;
			}
			else // This is a network download, but the network system isn't enabled so it will never complete
			{
				InternalFreeData();

				// Swap an invalid entry into the slot.
				m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<DataDefinition>());

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kError;
			}
		}

		// Cook the out of date file in developer builds.
		(void)CookManager::Get()->CookIfOutOfDate(GetFilePath());

		UInt32 const zMaxReadSize = kDefaultMaxReadSize;

		// If reading succeeds, continue on a worker thread.
		if (FileManager::Get()->ReadAll(
			GetFilePath(),
			m_pRawData,
			m_uDataSizeInBytes,
			kLZ4MinimumAlignment,
			MemoryBudgets::Content,
			zMaxReadSize))
		{
			// Finish the load on a worker thread.
			return Content::LoadState::kLoadingOnWorkerThread;
		}
	}
	// Second step, decompress the data.
	else if (Content::LoadState::kLoadingOnWorkerThread == GetContentLoadState())
	{
		void* pUncompressedFileData = nullptr;
		UInt32 uUncompressedFileDataInBytes = 0u;

		// Deobfuscate.
		Animation2D::Obfuscate((Byte*)m_pRawData, m_uDataSizeInBytes, GetFilePath());

		if (ZSTDDecompress(
			m_pRawData,
			m_uDataSizeInBytes,
			pUncompressedFileData,
			uUncompressedFileDataInBytes,
			MemoryBudgets::Content))
		{
			InternalFreeData();

			// Load.
			SharedPtr<DataDefinition> pData(SEOUL_NEW(MemoryBudgets::Rendering) DataDefinition(GetFilePath()));
			StreamBuffer buffer;
			buffer.TakeOwnership((Byte*)pUncompressedFileData, uUncompressedFileDataInBytes);
			Animation2D::ReadWriteUtil util(buffer, keCurrentPlatform);
			auto const bSuccess = util.BeginRead() && pData->Load(util);

			if (bSuccess)
			{
				m_hEntry.GetContentEntry()->AtomicReplace(pData);
				InternalReleaseEntry();

				// Done with loading body, decrement the loading count.
				return Content::LoadState::kLoaded;
			}
		}
	}

	// If we get here, an error occured, so cleanup and return the error condition.
	InternalFreeData();

	// Swap an invalid entry into the slot.
	m_hEntry.GetContentEntry()->AtomicReplace(SharedPtr<DataDefinition>());

	// Done with loading body, decrement the loading count.
	return Content::LoadState::kError;
}

void DataContentLoader::InternalFreeData()
{
	if (nullptr != m_pRawData)
	{
		MemoryManager::Deallocate(m_pRawData);
		m_pRawData = nullptr;
	}
	m_uDataSizeInBytes = 0u;
}

void DataContentLoader::InternalReleaseEntry()
{
	if (m_hEntry.IsInternalPtrValid())
	{
		// NOTE: We need to release our reference before decrementing the loader count.
		// This is safe, because a Content::Entry's Content::Store always maintains 1 reference,
		// and does not release it until the content is done loading.
		Animation2DDataContentHandle::EntryType* pEntry = m_hEntry.GetContentEntry().GetPtr();
		m_hEntry.Reset();
		pEntry->DecrementLoaderCount();
	}
}

} // namespace Seoul::Animation2D

#endif // /#if SEOUL_WITH_ANIMATION_2D
