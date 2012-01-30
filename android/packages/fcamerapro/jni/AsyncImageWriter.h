#ifndef _ASYNCIMAGEWRITER_H
#define _ASYNCIMAGEWRITER_H

#include <FCam/Tegra.h>
#include <vector>
#include "WorkQueue.h"


class FileFormatDescriptor {
	// A to-do for NVidia folks: add compression and save settings
public:
	enum EFormats {
		EFormatJPEG, EFormatTIFF, EFormatDNG, EFormatRAW
	};

	FileFormatDescriptor(EFormats format, int quality = 80) : m_format(format), m_quality(quality) { }
	~FileFormatDescriptor(void) { }

	EFormats getFormat(void) {
		return m_format;
	}

	int getQuality(void) {
		return m_quality;
	}

private:

	EFormats m_format;
	int m_quality;
};

typedef void (*ASYNC_IMAGE_WRITER_CALLBACK)(void);

class ImageSet {
	friend class AsyncImageWriter;
public:

	void add(const FileFormatDescriptor &ff, const FCam::Frame &frame);

private:
	ImageSet(int id, const char *outputDirPrefix);
	~ImageSet(void);

	void dumpToFileSystem(ASYNC_IMAGE_WRITER_CALLBACK proc);

	std::vector<FCam::Frame> m_frames;
	std::vector<FileFormatDescriptor> m_frameFormat;
	const char *m_outputDirPrefix;
	const int m_fileId;
};

class AsyncImageWriter {
public:
	AsyncImageWriter(const char *outputDirPrefix);
	~AsyncImageWriter(void);

	ImageSet *newImageSet(void);

	void push(ImageSet *is);
	void setOnFileSystemChangedCallback(ASYNC_IMAGE_WRITER_CALLBACK cb);
	static void SetFreeFileId(int id);

private:
	char *m_outputDirPrefix;
	WorkQueue<ImageSet *> m_queue;
	ASYNC_IMAGE_WRITER_CALLBACK m_onChangedCallback;

	pthread_t m_thread;
	static void *ThreadProc(void *);

	static int sFreeId;
};

#endif
