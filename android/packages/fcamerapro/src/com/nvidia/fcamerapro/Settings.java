package com.nvidia.fcamerapro;

public interface Settings {
	final static public String STORAGE_DIRECTORY = "DCIM/fcam";
	// XXX: the name pattern is also defined in AsyncImageWriter.cpp 
	// so any modification needs to be applied in both locations.
	// In addition, FCameraPROActivity.onCreate(...) also has knowledge
	// of this pattern. Next version should try to decouple them.
	final static public String IMAGE_STACK_PATTERN = "img_\\d{4}.xml";
		
	final static public int UI_MAX_FPS = 15;
	final static public int PREVIEW_WINDOW_MAX_FPS = 60;
	
	final static public int SEEK_BAR_PRECISION = 1000;
	final static public double SEEK_BAR_EXPOSURE_GAMMA = 5.0;
	final static public double SEEK_BAR_GAIN_GAMMA = 2.0;
	final static public double SEEK_BAR_FOCUS_GAMMA = 0.5;
	final static public double SEEK_BAR_WB_GAMMA = 1.0;
	
	final static public float BUSY_ICON_SCALE = 0.1f;
	final static public float BUSY_ICON_SPEED = 0.2f;
}
