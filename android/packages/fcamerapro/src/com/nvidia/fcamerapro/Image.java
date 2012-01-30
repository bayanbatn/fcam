package com.nvidia.fcamerapro;

import java.io.File;

import org.xml.sax.Attributes;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/**
 * Image class represents a captured photograph that has been saved to disk.
 */
public final class Image implements HistogramDataProvider {
	final private static int HISTOGRAM_SAMPLES = 32768;

	private Bitmap mThumbnail;
	final private String mThumbnailName;
	final private String mImageName;
	final private boolean mFlashOn;
	final private int mGain;
	final private int mExposure;
	final private int mWb;

	private float[] mHistogramData;

	public Image(String galleryDir, Attributes attributes) {
		mImageName = galleryDir + File.separatorChar + attributes.getValue("name");
		mThumbnailName = galleryDir + File.separatorChar + attributes.getValue("thumbnail");
		mFlashOn = Integer.parseInt(attributes.getValue("flash")) != 0;
		mGain = Integer.parseInt(attributes.getValue("gain"));
		mExposure = Integer.parseInt(attributes.getValue("exposure"));
		mWb = Integer.parseInt(attributes.getValue("wb"));
	}

	/* ====================================================================
	 * Load the image thumbnail and calculate histogram. 
	 * ==================================================================== */
	public boolean loadThumbnail() {
		// Return if the thumbnail has been loaded.
		if (mThumbnail != null) return true;
		// Load thumbnail.
		BitmapFactory.Options opts = new BitmapFactory.Options();
		opts.inScaled = false;
		mThumbnail = BitmapFactory.decodeFile(mThumbnailName, opts);
		// Return if loading failed.
		if (mThumbnail == null) return false;
		// Calculate histogram data by subsampling the thumbnail.
		int[] accum = new int[256];
		int[] buffer = new int[mThumbnail.getWidth() * mThumbnail.getHeight()];
		mThumbnail.getPixels(buffer, 0, mThumbnail.getWidth(), 0, 0,
				mThumbnail.getWidth(), mThumbnail.getHeight());
		int tx = 0;
		int ax = (buffer.length << 12) / HISTOGRAM_SAMPLES;
		for (int i = 0; i < HISTOGRAM_SAMPLES; i++, tx+=ax) {
			int pixel = buffer[tx >>> 12];
			int y = (13932 * ((pixel >> 16) & 0xff) // luma = 0.2126 R
					+ 46971 * ((pixel >> 8) & 0xff) //        + 0.7152 G
					+ 4733 * (pixel & 0xff)) >> 16; //        + 0.0722 B
			// XXX: Not quite correct actually. sRGB should be linearized
			// before applying this formula.
			accum[y]++;
		}
		// Normalize the histogram data by the largest bin.
		int maxv = 0;
		for (int i = 0; i < accum.length; maxv = Math.max(maxv, accum[i++]));
		float norm = maxv > 0 ? (1.0f / maxv) : 0.0f;
		mHistogramData = new float[accum.length];
		for (int i = 0; i < accum.length; mHistogramData[i] = accum[i++] * norm);
		return true;
	}

	/* ====================================================================
	 * Getters 
	 * ==================================================================== */
	public Bitmap getThumbnail() { return mThumbnail; }

	public String getThumbnailName() { return mThumbnailName; }
	
	public String getName() { return mImageName; }
	
	public String getInfo() {
		String str = Utils.GetFileName(mImageName);
		str += "\n" + Utils.FormatExposure(mExposure);
		str += "\n" + Utils.FormatGain(mGain / 100);
		str += "\n" + Utils.FormatWhiteBalance(mWb);
		str += "\n" + (mFlashOn ? "On" : "Off");
		return str;
	}

	/* ====================================================================
	 * Implementation of HistogramDataProvider interface.
	 * ==================================================================== */
	public void getHistogramData(float[] data) {
		if (mHistogramData != null) {
			System.arraycopy(mHistogramData, 0, data, 0, mHistogramData.length);
		}
	}

}
