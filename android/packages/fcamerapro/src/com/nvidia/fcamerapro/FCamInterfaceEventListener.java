package com.nvidia.fcamerapro;

/**
 * A listener interface for objects wishing to be notified of FCam events by
 * FCamInterface class. Currently implemented by ViewerFragment and
 * CameraViewRenderer (in CameraView.java.)
 * 
 * @see com.nvidia.fcamerapro.FCamInterface
 * @see com.nvidia.fcamerapro.ViewerFragment
 * @see com.nvidia.fcamerapro.CameraViewRenderer
 */
public interface FCamInterfaceEventListener {
	public void onCaptureStart();
	public void onCaptureComplete();
	public void onFileSystemChange();
	public void onShotParamChange(int shotParamId);
}
