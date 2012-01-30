#ifndef _PARAMSETREQUEST_H
#define _PARAMSETREQUEST_H

#include "Common.h"

#define HISTOGRAM_SIZE 256

/* [CS478]
 * These are allowed message types (for requests that are processed
 * by the work thread.)
 *
 * PARAM_PREVIEW_* relates to parameters for the viewfinder images.
 * PARAM_PREVIEW_AUTO_*_ON indicates that the request is asking for auto-* to be turned
 * on or off. These messages are generated as a result of user clicking the appropriate
 * checkboxes.
 *
 * PARAM_TAKE_PICTURE indicates that the request is asking for a shot to be taken.
 *
 * Others should be self-explanatory.
 *
 * See the switch-case statement in the main loop of the work thread, to see how
 * they are parsed.
 */
#define PARAM_SHOT                     0
#define PARAM_RESOLUTION               1
#define PARAM_BURST_SIZE               2
#define PARAM_OUTPUT_FORMAT            3
#define PARAM_VIEWER_ACTIVE            4
#define PARAM_OUTPUT_DIRECTORY         5
#define PARAM_OUTPUT_FILE_ID           6
#define PARAM_LUMINANCE_HISTOGRAM      7
#define PARAM_PREVIEW_EXPOSURE         8
#define PARAM_PREVIEW_FOCUS            9
#define PARAM_PREVIEW_GAIN             10
#define PARAM_PREVIEW_WB               11
#define PARAM_PREVIEW_AUTO_EXPOSURE_ON 12
#define PARAM_PREVIEW_AUTO_FOCUS_ON    13
#define PARAM_PREVIEW_AUTO_GAIN_ON     14
#define PARAM_PREVIEW_AUTO_WB_ON       15
#define PARAM_CAPTURE_FPS              16
#define PARAM_TAKE_PICTURE             17
/* [CS478]
 * The constants above are message types parsed by the work thread.
 * (Go look at the giant switch-case statement in FCamInterface.cpp.)
 * You can define new constants here, and add an appropriate handling
 * to the switch-case statement, in order to define new types of
 * messages, if needed.
 */
// TODO TODO TODO
// TODO TODO TODO
// TODO TODO TODO
// TODO TODO TODO
// TODO TODO TODO
#define PARAM_PRIV_FS_CHANGED     100

#define SHOT_PARAM_EXPOSURE 0
#define SHOT_PARAM_FOCUS    1
#define SHOT_PARAM_GAIN     2
#define SHOT_PARAM_WB       3
#define SHOT_PARAM_FLASH    4

class ParamSetRequest {
public:
	ParamSetRequest(void) {
		m_id = -1;
		m_dataSize = 0;
		m_data = 0;
	}

	ParamSetRequest(int param, const void *data, int dataSize) {
		m_id = param;
		m_dataSize = dataSize;
		m_data = new uchar[m_dataSize];
		if (data != 0) memcpy(m_data, data, m_dataSize);
	}

	ParamSetRequest(const ParamSetRequest &instance) {
		// copy constructor
		m_id = instance.m_id;
		m_dataSize = instance.m_dataSize;
		m_data = new uchar[m_dataSize];
		memcpy(m_data, instance.m_data, m_dataSize);
	}

	ParamSetRequest &operator=(const ParamSetRequest &instance) {
		m_id = instance.m_id;
		m_dataSize = instance.m_dataSize;
		if (m_data != 0) {
			delete [] m_data;
		}

		m_data = new uchar[m_dataSize];
		memcpy(m_data, instance.m_data, m_dataSize);

		return *this;
	}

	~ParamSetRequest(void) {
		if (m_data != 0) {
			delete [] m_data;
		}
	}

	int getId(void) {
		return m_id;
	}

	uchar *getData(void) {
		return m_data;
	}

	int getDataAsInt(void) {
		return ((int *) m_data)[0];
	}

	int getDataSize(void) {
		return m_dataSize;
	}

private:
	int m_id;
	uchar *m_data; // serialized param value
	int m_dataSize;
};


#endif


