package com.nvidia.fcamerapro;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.util.AttributeSet;

/**
 * Represents the viewfinder screen. Uses OpenGL ES.
 */
public final class CameraView extends GLSurfaceView {

	final private CameraViewRenderer mRenderer;

	public CameraView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setEGLConfigChooser(8, 8, 8, 8, 0, 0);
		
		mRenderer = new CameraViewRenderer(context);
		setRenderer(mRenderer);
	}

	public CameraViewRenderer getRenderer() {
		return mRenderer;
	}

	protected void onAttachedToWindow() {
		FCamInterface instance = FCamInterface.GetInstance();
		// If picture grab was completed while in viewer window, notify renderer.
		if (!instance.isCapturing()) mRenderer.onCaptureComplete();

		instance.setPreviewActive(true);
		instance.addEventListener(mRenderer);
		super.onAttachedToWindow();
	}

	protected void onDetachedFromWindow() {
		FCamInterface instance = FCamInterface.GetInstance();
		instance.setPreviewActive(false);
		instance.removeEventListener(mRenderer);
		super.onDetachedFromWindow();
	}
}

/**
 * An auxiliary class that performs the OpenGL render.
 */
final class CameraViewRenderer implements GLSurfaceView.Renderer, FCamInterfaceEventListener {
	final private FloatBuffer mVertexBuffer, mTextureBuffer;
	final private Context mContext;
	private long mBusyTimerStart = -1;
	private int mBusyTextureId = -1;
	private int mSurfaceWidth, mSurfaceHeight;

	final static private float sVertexCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
	final static private float sTextureCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };

	CameraViewRenderer(Context context) {
		mContext = context;

		ByteBuffer byteBuf = ByteBuffer.allocateDirect(sVertexCoords.length * 4);
		byteBuf.order(ByteOrder.nativeOrder());
		mVertexBuffer = byteBuf.asFloatBuffer();
		mVertexBuffer.put(sVertexCoords);
		mVertexBuffer.position(0);

		byteBuf = ByteBuffer.allocateDirect(sTextureCoords.length * 4);
		byteBuf.order(ByteOrder.nativeOrder());
		mTextureBuffer = byteBuf.asFloatBuffer();
		mTextureBuffer.put(sTextureCoords);
		mTextureBuffer.position(0);
	}

	@Override
	public void onDrawFrame(GL10 gl) {
		FCamInterface iface = FCamInterface.GetInstance();

		if (iface.isPreviewActive()) {		
			// lock frame texture 
			int texId = iface.lockViewerTexture();
			int texTarget = iface.getViewerTextureTarget();		
			// bind texture
			gl.glEnable(texTarget);
			gl.glBindTexture(texTarget, texId);
			// draw textured quad
			gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
			gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
			gl.glVertexPointer(2, GL10.GL_FLOAT, 0, mVertexBuffer);
			gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTextureBuffer);

			gl.glLoadIdentity();
			gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, 4);
			// unbind texture
			gl.glBindTexture(texTarget, 0);
			gl.glDisable(texTarget);
			
			// Draw busy animation if necessary.
			if (mBusyTimerStart > -1) {
				float aratio = (float) mSurfaceWidth / mSurfaceHeight;
				gl.glEnable(GL10.GL_BLEND);
				gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE);

				gl.glEnable(GL10.GL_TEXTURE_2D);
				gl.glBindTexture(GL10.GL_TEXTURE_2D, mBusyTextureId);

				gl.glTranslatef(0.5f, 0.5f, 0.0f);
				gl.glScalef(Settings.BUSY_ICON_SCALE, Settings.BUSY_ICON_SCALE * aratio, 1.0f);
				gl.glRotatef((System.currentTimeMillis() - mBusyTimerStart) * Settings.BUSY_ICON_SPEED, 0.0f, 0.0f, -1.0f);
				gl.glTranslatef(-0.5f, -0.5f, 0.0f);

				gl.glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
				gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, 4);
				gl.glDisable(GL10.GL_BLEND);
			}

			gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
			gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);

			// flush render calls
			gl.glFlush();
			gl.glFinish();
			
			// unlock frame texture
			iface.unlockViewerTexture();			
		}
	}

	public void onSurfaceChanged(GL10 gl, int width, int height) {
		mSurfaceWidth = width;
		mSurfaceHeight = height;
		gl.glViewport(0, 0, width, height);
		gl.glMatrixMode(GL10.GL_PROJECTION);
		gl.glLoadIdentity();
		gl.glOrthof(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);
		gl.glMatrixMode(GL10.GL_MODELVIEW);
		gl.glLoadIdentity();
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		// Load the "Busy" image into texture. Nothing else to do.
		mBusyTextureId = loadTextureFromResource(gl, R.drawable.busy);		
	}

	// A helper function to load file into texture.
	private int loadTextureFromResource(GL10 gl, int resourceId) {
		// Load bitmap.
		BitmapFactory.Options opts = new BitmapFactory.Options();
		opts.inScaled = false;
		Bitmap bitmap = BitmapFactory.decodeResource(mContext.getResources(), resourceId, opts);

		// Create texture.
		int[] tid = new int[1];
		gl.glGenTextures(1, tid, 0);
		gl.glBindTexture(GL10.GL_TEXTURE_2D, tid[0]);
		gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
		gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
		gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
		gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
		GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGBA, bitmap, 0);

		// Clean up.
		bitmap.recycle();
		return tid[0];
	}
	
	/* ====================================================================
	 * Implementation of FCamInterfaceEventListener interface.
	 * ==================================================================== */
	public void onCaptureStart() {
		mBusyTimerStart = System.currentTimeMillis();
	}
	public void onCaptureComplete() {
		mBusyTimerStart = -1;
	}
	public void onFileSystemChange() {}
	public void onShotParamChange(int shotParamId) {}	
}
