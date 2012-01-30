package com.nvidia.fcamerapro;

/**
 * An object that can provide histogram data should implement this.
 * Currently, Image and CameraFragment implement this interface.
 * 
 * @see com.nvidia.fcamerapro.Image
 * @see com.nvidia.fcamerapro.CameraFragment
 */
public interface HistogramDataProvider {
	/**
	 * Copy the histogram data to the provided array. It is presumed that
	 * the array's size is at least 256.
	 * @param data destination array.
	 */
	public void getHistogramData(float[] data);
}
