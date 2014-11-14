#ifndef OSMPBF_PARSEHELPERS_H
#define OSMPBF_PARSEHELPERS_H
#include "osmfilein.h"
#include "primitiveblockinputadaptor.h"
#include <omp.h>
#include <mutex>
#include <thread>

namespace osmpbf {

///@processor (osmpbf::PrimitiveBlockInputAdaptor & pbi)
template<typename TPBI_Processor>
void parseFile(osmpbf::OSMFileIn & inFile, TPBI_Processor processor);

///@processor (osmpbf::PrimitiveBlockInputAdaptor & pbi)
///@readBlobCount number of blobs to work on in parallel. If this is set to zero then this will default to max(omp_get_num_procs(), 1)
template<typename TPBI_Processor>
void parseFileOmp(osmpbf::OSMFileIn& inFile, TPBI_Processor processor, uint32_t readBlobCount = 0);

///@processor (osmpbf::PrimitiveBlockInputAdaptor & pbi)
///@threadCount if this is set to zero then this will default to max(omp_get_num_procs(), 1) or max(std::thread::hardware_concurrency(), 1)
///@readBlobCount number of blobs to read by a blob
template<typename TPBI_Processor>
void parseFileCPPThreads(osmpbf::OSMFileIn& inFile, TPBI_Processor processor, uint32_t threadCount = 0, uint32_t readBlobCount = 1);


//definitions

template<typename TPBI_Processor>
void parseFile(OSMFileIn & inFile, TPBI_Processor processor) {
	osmpbf::PrimitiveBlockInputAdaptor pbi;
	while (inFile.parseNextBlock(pbi)) {
		if (pbi.isNull())
			continue;
		processor(pbi);
	}
}

template<typename TPBI_Processor>
void parseFileOmp(OSMFileIn & inFile, TPBI_Processor processor, uint32_t readBlobCount) {
	if (!readBlobCount) {
		readBlobCount = std::max<int>(omp_get_num_procs(), 1);
	}
	std::vector<osmpbf::BlobDataBuffer> pbiBuffers;
	bool processedFile = false;
	while (!processedFile) {
		pbiBuffers.clear();
		inFile.getNextBlocks(pbiBuffers, readBlobCount);
		uint32_t pbiCount = pbiBuffers.size();
		processedFile = (pbiCount < readBlobCount);
		#pragma omp parallel for schedule(dynamic)
		for(uint32_t i = 0; i < pbiCount; ++i) {
			osmpbf::PrimitiveBlockInputAdaptor pbi(pbiBuffers[i].data, pbiBuffers[i].availableBytes);
			pbiBuffers[i].clear();
			if (pbi.isNull()) {
				continue;
			}
			processor(pbi);
		}
	}
}

template<typename TPBI_Processor>
void parseFileCPPThreads(osmpbf::OSMFileIn& inFile, TPBI_Processor processor, uint32_t threadCount, uint32_t readBlobCount) {
	if (!threadCount) {
		threadCount = std::max<int>(std::thread::hardware_concurrency(), 1);
	}
	readBlobCount = std::max<uint32_t>(readBlobCount, 1);
	std::mutex mtx;
	auto workFunc = [&inFile, &processor, &mtx, readBlobCount]() {
		osmpbf::PrimitiveBlockInputAdaptor pbi;
		std::vector<osmpbf::BlobDataBuffer> dbufs;
		while (true) {
			dbufs.clear();
			mtx.lock();
			bool haveNext = inFile.getNextBlocks(dbufs, readBlobCount);
			mtx.unlock();
			if (!haveNext) {//nothing left to do
				break;
			}
			for(osmpbf::BlobDataBuffer & dbuf : dbufs) {
				pbi.parseData(dbuf.data, dbuf.availableBytes);
				processor(pbi);
			}
		}
	};
	std::vector<std::thread> ts;
	ts.reserve(threadCount);
	for(uint32_t i(0); i < threadCount; ++i) {
		ts.push_back(std::thread(workFunc));
	}
	for(std::thread & t : ts) {
		t.join();
	}
}


}//end namespace

#endif