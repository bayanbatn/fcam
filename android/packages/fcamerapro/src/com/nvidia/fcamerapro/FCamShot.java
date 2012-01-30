package com.nvidia.fcamerapro;

/**
 * a simple JAVA-side abstraction for a FCam::Shot object.
 */
public final class FCamShot {
	public double exposure, gain, wb, focus;
	public boolean flashOn;
	
	public FCamShot clone() {
		FCamShot instance = new FCamShot();
		
		instance.exposure = exposure;
		instance.gain = gain;
		instance.wb = wb;
		instance.focus = focus;
		instance.flashOn = flashOn;
		
		return instance;
	}
}
