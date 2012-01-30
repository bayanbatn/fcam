package com.nvidia.fcamerapro;

import java.io.File;

/**
 * Utility methods for formatting camera parameters.
 */
public final class Utils {
	public static String FormatExposure(double exposure) {
		// Convert the exposure to seconds
		double val = exposure / 1000000;

		if (val < 0.001) {
			return String.format("1/%d000s", (int) (0.001 / val + 0.5));
		} else if (val < 0.01) {
			return String.format("1/%d00s", (int) (0.01 / val + 0.5));
		} else if (val < 0.1) {
			return String.format("1/%d0s", (int) (0.1 / val + 0.5));
		} else if (val < 0.2) {
			return String.format("1/%ds", (int) (1.0 / val + 0.5));
		} else if (val < 0.95) {
			return String.format("0.%ds", (int) (10 * val + 0.5));
		} else {
			return String.format("%ds", (int) (val + 0.5));
		}
	}

	public static String FormatGain(double gain) {
		return String.format("ISO%d00", (int) gain);
	}

	public static String FormatFocus(double focus) {
		if (focus > 5.0) { // up to 20cm
			return String.format("%dcm", (int) (100 / focus + 0.5));
		} else if (focus > 1.0) { // up to 1m
			return String.format("%d0cm", (int) (10 / focus + 0.5));
		} else if (focus > 0.2) { // up to 5m
			return String.format("%dm", (int) (1 / focus + 0.5));
		} else {
			return String.format(">5m");
		}
	}

	public static String FormatWhiteBalance(double wb) {
		return String.format("%d00K", (int) (wb / 100 + 0.5));
	}
	
	public static String GetFileName(String name) {
		return name.substring(name.lastIndexOf(File.separatorChar) + 1);
	}
}
