package com.nvidia.fcamerapro;

import android.content.Context;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * A UI widget that displays a histogram.
 */
public final class HistogramView extends View implements OnClickListener {
	final static private int DRAW_MODE_COARSE_ACCUM_COLUMNS = 4;
	private enum DrawMode {
		NORMAL, COARSE;
	}

	// Normalized histogram data
	private final float[] mBinData = new float[256];

	private HistogramDataProvider mDataProvider;
	private Paint mHistogramPaint, mBackgroundPaint;
	private DrawMode mDrawMode;
	final private boolean mAutoRefresh;

	// Update timer handler
	private Handler mHandler = new Handler();

	// Make a new Runnable instance that will repeatedly redraw the histogram.
	private Runnable mUpdateUITask = new Runnable() {
		public void run() {
			invalidate();
			mHandler.postDelayed(this, 1000 / Settings.UI_MAX_FPS);
		}
	};

	public void setDataProvider(HistogramDataProvider provider) {
		mDataProvider = provider;
		if (provider == null)
			for (int i = 0; i < mBinData.length; mBinData[i++] = 0.0f);
		postInvalidate();
	}

	public HistogramView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setOnClickListener(this); // Handle clicks.

		setBackgroundColor(Color.TRANSPARENT);

		mHistogramPaint = new Paint();
		mHistogramPaint.setColor(0xff707080);

		mBackgroundPaint = new Paint();
		mBackgroundPaint.setColor(0x40707080);
		mBackgroundPaint.setStyle(Style.STROKE);
		mBackgroundPaint.setStrokeWidth(2);

		mDrawMode = attrs.getAttributeBooleanValue(null, "draw_coarse", false) ? DrawMode.COARSE : DrawMode.NORMAL;
		mAutoRefresh = attrs.getAttributeBooleanValue(null, "auto_refresh", false);
	}

	protected void onDraw(Canvas canvas) {
		if (canvas == null) return;
		float width = canvas.getWidth();
		float height = canvas.getHeight();
		float ax = 0.0f; // starting horizontal location
		float dx = width / mBinData.length; // width per bin

		// Clear canvas.
		canvas.drawRect(0.0f, 0.0f, width, height, mBackgroundPaint);
		// Fetch histogram data.
		if (mDataProvider != null) mDataProvider.getHistogramData(mBinData);
		// Draw the individual bars.
		switch (mDrawMode) {
		case NORMAL:
			for (int i = 0; i < mBinData.length; i++) {
				canvas.drawRect(ax, height - height * mBinData[i],
						ax + dx, height, mHistogramPaint);
				ax += dx;
			}
			break;
		case COARSE:
			dx *= DRAW_MODE_COARSE_ACCUM_COLUMNS;
			for (int i = 0; i < mBinData.length; i += DRAW_MODE_COARSE_ACCUM_COLUMNS) {
				float avalue = mBinData[i];
				// Currently, the "coarse" mode will display the largest bin value out of
				// DRAW_MODE_COARSE_ACCUM_COLUMNS-many bins. One could alternatively
				// display the average or median, etc.
				for (int j = 1; j < DRAW_MODE_COARSE_ACCUM_COLUMNS; j++)
					avalue = Math.max(mBinData[i + j], avalue);
				canvas.drawRect(ax, height - height * avalue,
						ax + dx - 1, height, mHistogramPaint);
				ax += dx;
			}
			break;
		}
	}

	// Clicking on the histogram will toggle between coarse/normal view.
	public void onClick(View v) {
		switch (mDrawMode) {
		case NORMAL:
			mDrawMode = DrawMode.COARSE;
			break;
		case COARSE:
			mDrawMode = DrawMode.NORMAL;
			break;
		}
		invalidate();
	}

	// Upon attaching to a window, set up auto-refresh if applicable.
	protected void onAttachedToWindow() {
		if (mAutoRefresh) {
			mHandler.postDelayed(mUpdateUITask, 0);
		}
	}

	// Upon detaching to a window, remove the auto-refresh task if applicable.
	protected void onDetachedFromWindow() {
		if (mAutoRefresh) {
			mHandler.removeCallbacks(mUpdateUITask);
		}
	}
}
